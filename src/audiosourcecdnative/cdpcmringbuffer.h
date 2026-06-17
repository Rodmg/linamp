#ifndef CDPCMRINGBUFFER_H
#define CDPCMRINGBUFFER_H

#include <QByteArray>
#include <QMutex>

class CDPcmRingBuffer
{
public:
    explicit CDPcmRingBuffer(int capacityBytes = 2352 * 256);

    int write(const char *data, int length);
    int read(char *target, int maxLength);
    void clear();

    int size() const;
    int capacity() const;

private:
    mutable QMutex m_mutex;
    QByteArray m_storage;
    int m_capacity = 0;
    int m_head = 0;
    int m_tail = 0;
    int m_size = 0;
};

#endif // CDPCMRINGBUFFER_H
