#include "systemaudiocapture_pulseaudio.h"
#include "util.h"
#include "spectrumwidget.h"

#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

QMutex *sampleMutex;
QByteArray *sample;
QDataStream *sampleStream;

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
    if(actualbytes >= DFT_SIZE * 4) {
        // If the sample is equal or bigger than the target
        // Replace the current buffer
        sample->clear();
        delete sampleStream;
        sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    }
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
    // Limit max data so spectrumwidget paints frequently
    bufferAttr.maxlength = MAX_AUDIO_STREAM_SAMPLE_SIZE;
    bufferAttr.fragsize = MAX_AUDIO_STREAM_SAMPLE_SIZE;
    if (pa_stream_connect_record(stream, monitor_name.c_str(), &bufferAttr, PA_STREAM_ADJUST_LATENCY) != 0) {
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

bool isValidSample(QByteArray *sample)
{
    // Greedy: suposing that 100 is enough
    for(int i = 0; i < 100; i++) {
        if(sample->at(i) != 0) {
            return true;
        }
    }
    return false;
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
