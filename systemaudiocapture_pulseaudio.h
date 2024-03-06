#ifndef SYSTEMAUDIOCAPTURE_PULSEAUDIO_H
#define SYSTEMAUDIOCAPTURE_PULSEAUDIO_H

#include <QtConcurrent>

#define PA_SAMPLE_RATE DEFAULT_SAMPLE_RATE
#define PA_CHANNELS 2

bool isValidSample(QByteArray *sample);
int startPACapture();
void stopPACapture();

#endif // SYSTEMAUDIOCAPTURE_PULSEAUDIO_H
