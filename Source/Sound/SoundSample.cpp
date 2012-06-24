#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"
#include "Interpolator.h"

static bool Configure(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id)
{
	// sample length
	float length = 0;
	element->QueryFloatAttribute("length", &length);
	int samples = xs_CeilToInt(length * AUDIO_FREQUENCY);

	// reserve space
	self.Reserve(samples);

	// clock frequency
	int frequency = 0;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 1;
	element->QueryFloatAttribute("divider", &divider);

	// amplitude
	float amplitude = 1.0f;
	element->QueryFloatAttribute("amplitude", &amplitude);

	// offset
	float offset = 0.0f;
	element->QueryFloatAttribute("offset", &offset);

	// quantization
	float samplequant = 1.0f / 65536.0f;
	element->QueryFloatAttribute("quantize", &samplequant);

	// get samples
	float sample = 0.0f;
	std::vector<unsigned int> samplekey;
	const char *names[1] = { "value" };
	ApplyInterpolatorFunc samplefunc = ApplyConstant;
	if (ConfigureInterpolatorItem(element, samplekey, 1, names, &sample))
	{
		int interpolate = 1;
		element->QueryIntAttribute("interpolate", &interpolate);
		samplefunc = interpolate ? ApplyInterpolator : ApplyInterpolatorConstant;
	}
	if (samplekey.empty())
	{
		samplekey.reserve(3);
		samplekey.push_back(1);
		samplekey.push_back(0);
		samplekey.push_back(*reinterpret_cast<unsigned int *>(&sample));
		samplefunc = ApplyConstant;
	}

	const int oversample = 4;

	float time = 0.0f;
	const float steptime = float(frequency) / float(divider * AUDIO_FREQUENCY * oversample);
	int samplehint = 0;

	// for each sample...
	for (int i = 0; i < samples; ++i)
	{
		// for each oversample...
		float accum = 0;
		for (int j = 0; j < oversample; ++j)
		{
			// get current sample value
			float sample;
			samplefunc(&sample, 1, samplekey[0], reinterpret_cast<const float * __restrict>(&samplekey[1]), time, samplehint);
			sample = xs_RoundToInt(sample / samplequant) * samplequant;

			// accumulate value
			accum += offset + amplitude * sample;

			// advance time
			time += steptime;
		}

		// append sample
		self.Append(short(Clamp(xs_RoundToInt(SHRT_MAX * accum / oversample), SHRT_MIN, SHRT_MAX)));
	}

	return true;
}

static SoundConfigure::Configure soundsample(0x96e382a7 /* "sample" */, Configure);
