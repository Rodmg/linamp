#ifndef CDPCMIODEVICE_H
#define CDPCMIODEVICE_H

#include <QIODevice>

class CDPcmRingBuffer;

class CDPcmIODevice : public QIODevice
{
    Q_OBJECT
public:
    explicit CDPcmIODevice(CDPcmRingBuffer *ringBuffer, QObject *parent = nullptr);

    bool isSequential() const override;
    bool seek(qint64 pos) override;
    bool atEnd() const override;
    qint64 bytesAvailable() const override;
    void notifyReadyRead();

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private:
    CDPcmRingBuffer *m_ringBuffer = nullptr;
};

#endif // CDPCMIODEVICE_H
