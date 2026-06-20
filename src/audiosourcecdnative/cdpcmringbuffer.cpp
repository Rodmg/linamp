#include "cdpcmringbuffer.h"

#include <algorithm>

CDPcmRingBuffer::CDPcmRingBuffer(int capacityBytes)
    : m_capacity(capacityBytes)
{
    if (m_capacity < 1) {
        m_capacity = 1;
    }
    m_storage.resize(m_capacity);
}

int CDPcmRingBuffer::write(const char *data, int length)
{
    if (data == nullptr || length <= 0) {
        return 0;
    }

    QMutexLocker lock(&m_mutex);
    const int writable = std::min(length, m_capacity - m_size);
    if (writable <= 0) {
        return 0;
    }

    const int firstChunk = std::min(writable, m_capacity - m_tail);
    memcpy(m_storage.data() + m_tail, data, firstChunk);
    const int secondChunk = writable - firstChunk;
    if (secondChunk > 0) {
        memcpy(m_storage.data(), data + firstChunk, secondChunk);
    }

    m_tail = (m_tail + writable) % m_capacity;
    m_size += writable;
    return writable;
}

int CDPcmRingBuffer::read(char *target, int maxLength)
{
    if (target == nullptr || maxLength <= 0) {
        return 0;
    }

    QMutexLocker lock(&m_mutex);
    const int readable = std::min(maxLength, m_size);
    if (readable <= 0) {
        return 0;
    }

    const int firstChunk = std::min(readable, m_capacity - m_head);
    memcpy(target, m_storage.constData() + m_head, firstChunk);
    const int secondChunk = readable - firstChunk;
    if (secondChunk > 0) {
        memcpy(target + firstChunk, m_storage.constData(), secondChunk);
    }

    m_head = (m_head + readable) % m_capacity;
    m_size -= readable;
    return readable;
}

void CDPcmRingBuffer::clear()
{
    QMutexLocker lock(&m_mutex);
    m_head = 0;
    m_tail = 0;
    m_size = 0;
}

int CDPcmRingBuffer::size() const
{
    QMutexLocker lock(&m_mutex);
    return m_size;
}

int CDPcmRingBuffer::capacity() const
{
    return m_capacity;
}
