#include "cdpcmiodevice.h"

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

qint64 CDPcmIODevice::readData(char *data, qint64 maxSize)
{
    if (m_ringBuffer == nullptr || data == nullptr || maxSize <= 0) {
        return 0;
    }
    return m_ringBuffer->read(data, static_cast<int>(maxSize));
}

qint64 CDPcmIODevice::writeData(const char *, qint64)
{
    return -1;
}
