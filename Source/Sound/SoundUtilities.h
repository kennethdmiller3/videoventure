#pragma once

float OutputPulse(SoundTemplate &self, int ticks, float samplespertick, float samples, short value);

typedef bool (*ApplyInterpolatorFunc)(float target[], int width, int count, const float keys[], float aTime, int &aIndex);
bool ApplyConstant(float target[], int width, int count, const float keys[], float aTime, int &aIndex);
