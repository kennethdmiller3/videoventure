#include "StdAfx.h"
#include "Sound.h"
#include "SoundMixer.h"

#if defined(USE_SDL) && !defined(USE_SDL_MIXER)

#define DISTANCE_FALLOFF
extern float SOUND_DISTANCE_FACTOR;
extern float SOUND_ROLLOFF_FACTOR;
extern float SOUND_DOPPLER_FACTOR;
extern float CAMERA_DISTANCE;


// AUDIO MIXER

static const float timestep = 1.0f / AUDIO_FREQUENCY;
static float average0 = 0.0f, average1 = 0.0f;
static const float averagefilter = 1.0f * timestep;
static const float minlevel = 32768.0f*32768.0f;
static float level = minlevel;
static const float levelfilter = 1.0f * timestep;
static const float postscale = 32767.0f;

inline float SoftClamp(float x)
{
	float exp2x(expf(x+x));
	return (exp2x - 1) / (exp2x + 1);
}

void MixSound(void *userdata, unsigned char *stream, int len)
{
	int samples = len / sizeof(short);

	// if no sounds playing...
	if (sHead == NULL)
	{
		// update filters
		average0 -= average0 * 0.5f * samples * averagefilter;
		average1 -= average1 * 0.5f * samples * averagefilter;
		level -= level * 0.5f * samples * levelfilter;
		if (level < minlevel)
			level = minlevel;
		return;
	}

#ifdef PROFILE_SOUND_MIXER
	LARGE_INTEGER perf0;
	QueryPerformanceCounter(&perf0);
#endif

	// custom mixer
	float *mix = static_cast<float *>(_alloca(samples * sizeof(float)));
	memset(mix, 0, samples * sizeof(float));

	// listener position
	Vector2 &listenerpos = *static_cast<Vector2 *>(userdata);

	// sound channels
	struct ChannelInfo
	{
		float weight;
		float volume;
		const short *data;
		unsigned int offset;
		unsigned int length;
		unsigned int repeat;
	};
	ChannelInfo *channel_info = static_cast<ChannelInfo *>(_alloca((SOUND_CHANNELS+1) * sizeof(ChannelInfo)));
	memset(channel_info, 0, (SOUND_CHANNELS+1) * sizeof(ChannelInfo));
	int channel_count = 0;

	// for each active sound...
	for (Sound *sound = sHead; sound != NULL; sound = sound->mNext)
	{
		// get sound data
		const short *data = static_cast<short *>(sound->mData);
		unsigned int offset = sound->mOffset;
		unsigned int length = sound->mLength;
		unsigned int repeat = sound->mRepeat;

		// update sound position
		sound->mOffset += samples / 2;

		// if moving past the end...
		if (sound->mOffset >= length)
		{
			// if repeating
			if (repeat)
			{
				// loop the sound
				sound->mOffset %= length;
			}
			else
			{
				// clamp to the end
				sound->mOffset = length;
			}
		}

		// done if producing no output
		if (SOUND_CHANNELS <= 0)
			continue;

		// get intrinsic volume
		float volume = sound->mVolume;

#if defined(DISTANCE_FALLOFF)
		// if associated with an identifier, and applying rolloff
		if (sound->mId && SOUND_ROLLOFF_FACTOR)
		{
			// get distance
			const float dist = sqrtf(listenerpos.DistSq(mPosition) + CAMERA_DISTANCE * CAMERA_DISTANCE);
			const float mNear = CAMERA_DISTANCE;

			// apply sound fall-off
			volume *= mNear / (mNear + SOUND_ROLLOFF_FACTOR * (dist - mNear));
			if (volume < 1.0f/256.0f)
				continue;
		}
#endif

		// weight sound based on volume
		float weight = volume;

		// if not repeating...
		if (!repeat)
		{
			// diminish weight over time
			weight *= 1.0f - 0.5f * float(offset) / float(length);
		}

		int j;

		bool merge = false;
		for (j = 0; j < channel_count; j++)
		{
			// if the sound is a duplicate...
			if (channel_info[j].data == data && channel_info[j].offset == offset && channel_info[j].length == length && channel_info[j].repeat == repeat)
			{
				// merge with the existing sound
				volume = std::max(channel_info[j].volume, volume);
				weight = (channel_info[j].weight + weight);
				merge = true;
				break;
			}
		}

		// move lower-weight channels up
		for (; j > 0 && channel_info[j - 1].weight < weight; j--)
		{
			channel_info[j] = channel_info[j - 1];
		}

		// if room for the new sound...
		if (j < SOUND_CHANNELS)
		{
			// insert new sound
			channel_info[j].weight = weight;
			channel_info[j].volume = volume;
			channel_info[j].data = data;
			channel_info[j].offset = offset;
			channel_info[j].length = length;
			channel_info[j].repeat = repeat;

			// if not merging, and not out of channels...
			if (!merge && channel_count < SOUND_CHANNELS)
			{
				// bump the channel count
				++channel_count;
			}
		}
	}

	// for each active channel...
	for (int channel = 0; channel < channel_count; ++channel)
	{
		// get starting offset
		float volume = channel_info[channel].volume * SOUND_VOLUME_EFFECT;
		const short *data = channel_info[channel].data;
		unsigned int length = channel_info[channel].length;
		unsigned int offset = channel_info[channel].offset;

		// while output to generate...
		const short *src = data + offset;
		const short *srcend = data + length;
		float *dst = mix;
		const float *dstend = mix + samples;
		while (dst < dstend)
		{
			// add volume-scaled samples
			// (lesser of remaining destination and remaining source)
#if 1
			while (dst < dstend && src < srcend)
			{
				*dst++ += float(*src) * volume;
				*dst++ += float(*src) * volume;
				++src;
			}
#else
			// Duff's device :)
			register int count = std::min(dstend - dst, srcend - src);
			register int n = (count + 7) / 8;
			switch (count % 8)
			case 0: do { *dst++ += float(*src++) * volume;
			case 7:      *dst++ += float(*src++) * volume;
			case 6:      *dst++ += float(*src++) * volume;
			case 5:      *dst++ += float(*src++) * volume;
			case 4:      *dst++ += float(*src++) * volume;
			case 3:      *dst++ += float(*src++) * volume;
			case 2:      *dst++ += float(*src++) * volume;
			case 1:      *dst++ += float(*src++) * volume;
					   } while (--n > 0);
#endif

			// if reaching the end...
			if (src >= srcend)
			{
				// if repeating...
				if (channel_info[channel].repeat)
				{
					// loop around
					src -= length;
				}
				else
				{
					// stop
					break;
				}
			}
		}
	}

	// if generating output...
	if (SOUND_CHANNELS > 0)
	{
		// subract filtered average to remove DC term
		// apply filtered scaling to compress dynamic range
		// apply nonlinear curve to eliminate clipping
		const float *src = mix;
		const float *srcend = mix + samples;
		short *dst = (short *)stream;
		while (src < srcend)
		{
			float mix0 = *src++;
			float mix1 = *src++;
			mix0 -= average0;
			mix1 -= average1;
			average0 += mix0 * averagefilter;
			average1 += mix1 * averagefilter;
			level += (mix0 * mix0 + mix1 * mix1 - level) * levelfilter;
			if (level < minlevel)
				level = minlevel;
			float prescale = InvSqrt(level);
			*dst++ = short(SoftClamp(mix0 * prescale) * postscale);
			*dst++ = short(SoftClamp(mix1 * prescale) * postscale);
		}
	}
	else
	{
		// clear the output
		memset(stream, 0, len);
	}

#ifdef PROFILE_SOUND_MIXER
	LARGE_INTEGER perf1;
	QueryPerformanceCounter(&perf1);
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	DebugPrint("mix %d\n", 1000000*(perf1.QuadPart-perf0.QuadPart)/freq.QuadPart);
#endif
}

#endif
