#include "audiosourcebluetooth.h"
#include "spectrumwidget.h"

#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <QtConcurrent>

#define PA_SAMPLE_RATE 44100
#define PA_CHANNELS 2

static QMutex *sampleMutex;
static QByteArray *sample;
static QDataStream *sampleStream;
static bool captureRunning = false;
static pa_mainloop *paLoop;

static void pa_stream_notify_cb(pa_stream *stream, void* /*userdata*/)
{
    const pa_stream_state state = pa_stream_get_state(stream);
    switch (state) {
    case PA_STREAM_FAILED:
        qDebug() << "Stream failed";
        break;
    case PA_STREAM_READY:
        qDebug() << "Stream ready";
        break;
    default:
        qDebug() << "Stream state: " << state;
    }
}

static void pa_stream_read_cb(pa_stream *stream, const size_t /*nbytes*/, void* /*userdata*/)
{
    if(sampleMutex == nullptr || sample == nullptr || sampleStream == nullptr) {
        return;
    }
    QMutexLocker l(sampleMutex);

    // Careful when to pa_stream_peek() and pa_stream_drop()!
    // c.f. https://www.freedesktop.org/software/pulseaudio/doxygen/stream_8h.html#ac2838c449cde56e169224d7fe3d00824
    int16_t *data = nullptr;
    size_t actualbytes = 0;
    if (pa_stream_peek(stream, (const void**)&data, &actualbytes) != 0) {
        qDebug() << "Failed to peek at stream data";
        return;
    }

    if (data == nullptr && actualbytes == 0) {
        // No data in the buffer, ignore.
        return;
    } else if (data == nullptr && actualbytes > 0) {
        // Hole in the buffer. We must drop it.
        if (pa_stream_drop(stream) != 0) {
            qDebug() << "Failed to drop a hole! (Sounds weird, doesn't it?)";
            return;
        }
    }

    // process data
    //qDebug() << ">> " << actualbytes << " bytes";
    sampleStream->writeRawData((const char *)data, actualbytes);

    if (pa_stream_drop(stream) != 0) {
        qDebug() << "Failed to drop data after peeking.";
    }
}

static void pa_server_info_cb(pa_context *ctx, const pa_server_info *info, void* /*userdata*/)
{
    qDebug() << "Default sink: " << info->default_sink_name;

    pa_sample_spec spec;
    spec.format = PA_SAMPLE_S16LE;
    spec.rate = PA_SAMPLE_RATE;
    spec.channels = PA_CHANNELS;
    // Use pa_stream_new_with_proplist instead?
    pa_stream *stream = pa_stream_new(ctx, "output monitor", &spec, nullptr);

    pa_stream_set_state_callback(stream, &pa_stream_notify_cb, nullptr /*userdata*/);
    pa_stream_set_read_callback(stream, &pa_stream_read_cb, nullptr /*userdata*/);

    std::string monitor_name(info->default_sink_name);
    monitor_name += ".monitor";
    pa_buffer_attr bufferAttr;
    bufferAttr.maxlength = 4096;
    bufferAttr.fragsize = 4096;
    if (pa_stream_connect_record(stream, monitor_name.c_str(), &bufferAttr, PA_STREAM_NOFLAGS) != 0) {
        qDebug() << "connection fail";
        return;
    }

    qDebug() << "Connected to " << monitor_name.c_str();
}

static void pa_context_notify_cb(pa_context *ctx, void* /*userdata*/)
{
    const pa_context_state state = pa_context_get_state(ctx);
    switch (state) {
    case PA_CONTEXT_READY:
        qDebug() << "Context ready";
        pa_context_get_server_info(ctx, &pa_server_info_cb, nullptr /*userdata*/);
        break;
    case PA_CONTEXT_FAILED:
        qDebug() << "Context failed";
        break;
    default:
        qDebug() << "Context state: " << state;
    }
}

int startPACapture()
{
    captureRunning = true;

    paLoop = pa_mainloop_new();
    pa_mainloop_api *api = pa_mainloop_get_api(paLoop);
    pa_context *ctx = pa_context_new(api, "padump");
    pa_context_set_state_callback(ctx, &pa_context_notify_cb, nullptr /*userdata*/);
    if (pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        qDebug() << "PA connection failed.";
        return -1;
    }

    pa_mainloop_run(paLoop, nullptr);

    pa_context_disconnect(ctx);
    pa_mainloop_free(paLoop);
    return 0;
}

void stopPACapture()
{
    if(!captureRunning) {
        return;
    }

    pa_mainloop_quit(paLoop, 0);
    if(sample != nullptr) {
        sample->clear();
    }
    if(sampleStream) {
        delete sampleStream;
        sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    }
    captureRunning = false;
}

AudioSourceBluetooth::AudioSourceBluetooth(QObject *parent)
    : AudioSource{parent}
{
    sampleMutex = new QMutex();
    sample = new QByteArray();
    sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    dataEmitTimer = new QTimer(this);
    dataEmitTimer->setInterval(33); // around 30 fps
    connect(dataEmitTimer, &QTimer::timeout, this, &AudioSourceBluetooth::emitData);
}

AudioSourceBluetooth::~AudioSourceBluetooth()
{
    stopPACapture();
}

void AudioSourceBluetooth::activate()
{
    QMediaMetaData metadata = QMediaMetaData{};
    metadata.insert(QMediaMetaData::Title, "Bluetooth");

    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    emit metadataChanged(metadata);
    emit durationChanged(0);
    emit eqEnabledChanged(false);
    emit plEnabledChanged(false);
    emit shuffleEnabledChanged(false);
    emit repeatEnabledChanged(false);
}

void AudioSourceBluetooth::deactivate()
{
    stopPACapture();
    dataEmitTimer->stop();
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceBluetooth::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourceBluetooth::handlePrevious()
{

}

void AudioSourceBluetooth::handlePlay()
{
    QtConcurrent::run(startPACapture);
    dataEmitTimer->start();
    emit playbackStateChanged(MediaPlayer::PlayingState);
}

void AudioSourceBluetooth::handlePause()
{
    stopPACapture();
    dataEmitTimer->stop();
    emit playbackStateChanged(MediaPlayer::PausedState);
}

void AudioSourceBluetooth::handleStop()
{
    stopPACapture();
    dataEmitTimer->stop();
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceBluetooth::handleNext()
{

}

void AudioSourceBluetooth::handleOpen()
{

}

void AudioSourceBluetooth::handleShuffle()
{
    emit shuffleEnabledChanged(false);
}

void AudioSourceBluetooth::handleRepeat()
{
    emit repeatEnabledChanged(false);
}

void AudioSourceBluetooth::handleSeek(int mseconds)
{

}

void AudioSourceBluetooth::emitData()
{
    QMutexLocker l(sampleMutex);

    if(sample->length() < DFT_SIZE * 4) {
        return;
    }

    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(PA_SAMPLE_RATE);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setChannelCount(PA_CHANNELS);
    emit dataEmitted(*sample, format);
    sample->clear();
    delete sampleStream;
    sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
}
