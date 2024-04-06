#include "audiosourcewspectrumcapture.h"
#include "spectrumwidget.h"

#define SPECTRUM_DATA_SAMPLE_RATE 44100
#define SPECTRUM_DATA_CHANNELS 2

bool is_valid_sample(QByteArray *sample)
{
    // Greedy: suposing that 100 is enough
    for(int i = 0; i < 100; i++) {
        if(sample->at(i) != 0) {
            return true;
        }
    }
    return false;
}

/* our data processing function is in general:
 *
 *  struct pw_buffer *b;
 *  b = pw_stream_dequeue_buffer(stream);
 *
 *  .. consume stuff in the buffer ...
 *
 *  pw_stream_queue_buffer(stream, b);
 */
static void on_process(void *userdata)
{    
        struct PwData *data = (PwData*)userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
        qint16 *samples, max;
        uint32_t c, n, n_channels, n_samples, n_bytes, peak;

        QMutexLocker l(data->sampleMutex);


        if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }

        buf = b->buffer;
        if ((samples = (qint16*)buf->datas[0].data) == NULL)
                return;

        //n_channels = data->format.info.raw.channels;
        //n_samples = buf->datas[0].chunk->size / sizeof(qint16);
        n_bytes = buf->datas[0].chunk->size;


        if(n_bytes >= DFT_SIZE * 4) {
            // If the sample is equal or bigger than the target
            // Replace the current buffer
            data->sample->clear();
            delete data->sampleStream;
            data->sampleStream = new QDataStream(data->sample, QIODevice::WriteOnly);
        }
        data->sampleStream->writeRawData((const char *)samples, n_bytes);

        data->move = true;

        pw_stream_queue_buffer(data->stream, b);
}

/* Be notified when the stream param changes. We're only looking at the
 * format changes.
 */
static void
on_stream_param_changed(void *_data, uint32_t id, const struct spa_pod *param)
{
        struct PwData *data = (PwData*)_data;

        /* NULL means to clear the format */
        if (param == NULL || id != SPA_PARAM_Format)
                return;

        if (spa_format_parse(param, &data->format.media_type, &data->format.media_subtype) < 0)
                return;

        /* only accept raw audio */
        if (data->format.media_type != SPA_MEDIA_TYPE_audio ||
            data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
                return;

        /* call a helper function to parse the format for us. */
        spa_format_audio_raw_parse(param, &data->format.info.raw);

//        fprintf(stdout, "capturing rate:%d channels:%d\n",
//                        data->format.info.raw.rate, data->format.info.raw.channels);

}

static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .param_changed = on_stream_param_changed,
        .process = on_process,
};

static void do_quit(void *userdata, int signal_number)
{
        struct PwData *data = (PwData*)userdata;
        pw_main_loop_quit(data->loop);
}

AudioSourceWSpectrumCapture::AudioSourceWSpectrumCapture(QObject *parent)
    : AudioSource{parent}
{
    spectrumDataFormat.setSampleFormat(QAudioFormat::Int16);
    spectrumDataFormat.setSampleRate(SPECTRUM_DATA_SAMPLE_RATE);
    spectrumDataFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    spectrumDataFormat.setChannelCount(SPECTRUM_DATA_CHANNELS);

    pwData.sampleMutex = new QMutex();
    pwData.sample = new QByteArray();
    pwData.sampleStream = new QDataStream(pwData.sample, QIODevice::WriteOnly);

    dataEmitTimer = new QTimer(this);
    dataEmitTimer->setInterval(33); // around 30 fps
    connect(dataEmitTimer, &QTimer::timeout, this, &AudioSourceWSpectrumCapture::emitData);

    pwLoopThread = QtConcurrent::run(&AudioSourceWSpectrumCapture::pwLoop, this);

    qDebug() << "Compiled with libpipewire: " << pw_get_headers_version() << " Linked with libpipewire:" << pw_get_library_version();
}

AudioSourceWSpectrumCapture::~AudioSourceWSpectrumCapture()
{
    do_quit(&this->pwData, 1);
    pw_deinit();
}

void AudioSourceWSpectrumCapture::pwLoop()
{
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct pw_properties *props;
    struct spa_pod_builder b;
    b.data = buffer;
    b.size = sizeof(buffer);

    pw_init(nullptr, nullptr);

    pwData.loop = pw_main_loop_new(NULL);

    pw_loop_add_signal(pw_main_loop_get_loop(pwData.loop), SIGINT, do_quit, &pwData);
    pw_loop_add_signal(pw_main_loop_get_loop(pwData.loop), SIGTERM, do_quit, &pwData);

    props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                            PW_KEY_CONFIG_NAME, "client-rt.conf",
                            PW_KEY_MEDIA_CATEGORY, "Capture",
                            PW_KEY_MEDIA_ROLE, "Music",
                            NULL);
    pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true");

    pwData.stream = pw_stream_new_simple(
                            pw_main_loop_get_loop(pwData.loop),
                            "audio-capture",
                            props,
                            &stream_events,
                            &pwData);

    struct spa_audio_info_raw audio_info;
    audio_info.format = SPA_AUDIO_FORMAT_S16_LE;
    audio_info.channels = SPECTRUM_DATA_CHANNELS;
    audio_info.rate = SPECTRUM_DATA_SAMPLE_RATE;
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                            &audio_info);

    pw_stream_connect(pwData.stream,
                              PW_DIRECTION_INPUT,
                              PW_ID_ANY,
                              (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                              PW_STREAM_FLAG_MAP_BUFFERS |
                              PW_STREAM_FLAG_RT_PROCESS),
                              params, 1);

    pw_main_loop_run(pwData.loop);

    pw_stream_destroy(pwData.stream);
    pw_main_loop_destroy(pwData.loop);
    pw_deinit();
}


void AudioSourceWSpectrumCapture::emitData()
{
    QMutexLocker l(pwData.sampleMutex);

    if(pwData.sample->length() < DFT_SIZE * 4) {
        return;
    }

    if(!is_valid_sample(pwData.sample)) {
        return;
    }

    emit dataEmitted(*pwData.sample, spectrumDataFormat);
    pwData.sample->clear();
    delete pwData.sampleStream;
    pwData.sampleStream = new QDataStream(pwData.sample, QIODevice::WriteOnly);
}


void AudioSourceWSpectrumCapture::startSpectrum()
{
    dataEmitTimer->start();
}

void AudioSourceWSpectrumCapture::stopSpectrum()
{
    dataEmitTimer->stop();
}
