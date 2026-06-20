#include "cdpcmiodevice.h"

#include <QThread>

#include "cdpcmringbuffer.h"

CDPcmIODevice::CDPcmIODevice(CDPcmRingBuffer *ringBuffer, QObject *parent)
    : QIODevice(parent)
    , m_ringBuffer(ringBuffer)
{
}

bool CDPcmIODevice::isSequential() const
{
    return true;
}

bool CDPcmIODevice::seek(qint64 pos)
{
    // QAudioSink may rewind to 0 when restarting. Treat that as a no-op for a live stream.
    return pos == 0;
}

bool CDPcmIODevice::atEnd() const
{
    // CD playback is a live stream; temporary underruns are not EOF.
    return false;
}

qint64 CDPcmIODevice::bytesAvailable() const
{
    if (m_ringBuffer == nullptr) {
        return QIODevice::bytesAvailable();
    }

    return static_cast<qint64>(m_ringBuffer->size()) + QIODevice::bytesAvailable();
}

void CDPcmIODevice::notifyReadyRead()
{
    QMetaObject::invokeMethod(this, [this]() {
        emit readyRead();
    }, Qt::QueuedConnection);
}

qint64 CDPcmIODevice::readData(char *data, qint64 maxSize)
{
    if (m_ringBuffer == nullptr || data == nullptr || maxSize <= 0) {
        return 0;
    }

    // Give the CD reader thread a short chance to refill before signaling an underrun.
    if (m_ringBuffer->size() == 0) {
        for (int i = 0; i < 12 && m_ringBuffer->size() == 0; ++i) {
            QThread::msleep(1);
        }
    }

    return m_ringBuffer->read(data, static_cast<int>(maxSize));
}

qint64 CDPcmIODevice::writeData(const char *, qint64)
{
    return -1;
}
