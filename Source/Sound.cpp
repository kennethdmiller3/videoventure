#include "StdAfx.h"
#include "Sound.h"
#include "Entity.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// sound pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Sound));
void *Sound::operator new(size_t aSize)
{
	return pool.malloc();
}
void Sound::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif

namespace Database
{
	Typed<SoundTemplate> soundtemplate(0x1b5ef1be /* "soundtemplate" */);
	Typed<Sound *> sound(0x0e0d9594 /* "sound" */);

	namespace Loader
	{
		class SoundLoader
		{
		public:
			SoundLoader()
			{
				AddConfigure(0x0e0d9594 /* "sound" */, Entry(this, &SoundLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// open sound template
				SoundTemplate &sound = Database::soundtemplate.Open(aId);

				element->QueryFloatAttribute("volume", &sound.mVolume);

				for (const TiXmlElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0xaaea5743 /* "file" */:
						{
							const char *name = child->Attribute("name");
							if (name)
							{
								// load wave file data
								SDL_AudioSpec wave;
								Uint8 *data;
								Uint32 dlen;
								if ( !SDL_LoadWAV(name, &wave, &data, &dlen) )
								{
									DebugPrint("error loading sound file \"%s\": %s\n", name, SDL_GetError());
									continue;
								}

								// build audio conversion
								SDL_AudioCVT cvt;
								SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq,
														AUDIO_S16,   2,             AUDIO_FREQUENCY);

#if 1
								// replace sound data
								sound.mData = static_cast<unsigned char *>(realloc(sound.mData, dlen * cvt.len_mult));
								sound.mLength = 0;
#else
								// append sound data
								sound.mData = static_cast<unsigned char *>(realloc(sound.mData, sound.mLength + dlen * cvt.len_mult));
#endif
								memcpy(sound.mData + sound.mLength, data, dlen);
								cvt.buf = sound.mData + sound.mLength;
								cvt.len = dlen;

								// convert to final format
								SDL_ConvertAudio(&cvt);
								sound.mLength += cvt.len_cvt;
								sound.mData = static_cast<unsigned char *>(realloc(sound.mData, sound.mLength));

								// release wave file data
								SDL_FreeWAV(data);
							}
						}
					}
				}

				// close sound template
				Database::soundtemplate.Close(aId);
			}
		}
		soundloader;
	}
	namespace Initializer
	{
		class SoundInitializer
		{
		public:
			SoundInitializer()
			{
				AddActivate(0x1b5ef1be /* "soundtemplate" */, Entry(this, &SoundInitializer::Activate));
				AddDeactivate(0x1b5ef1be /* "soundtemplate" */, Entry(this, &SoundInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				SDL_LockAudio();
				const SoundTemplate &soundtemplate = Database::soundtemplate.Get(aId);
				Sound *sound = new Sound(soundtemplate, aId);
				Database::sound.Put(aId, sound);
				SDL_UnlockAudio();
				sound->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				SDL_LockAudio();
				if (const Sound *sound = Database::sound.Get(aId))
					delete sound;
				Database::sound.Delete(aId);
				SDL_UnlockAudio();
			}
		}
		soundinitializer;
	}
}

SoundTemplate::SoundTemplate(void)
: mData(NULL)
, mLength(0)
, mVolume(1.0f)
{
}

SoundTemplate::SoundTemplate(const SoundTemplate &aTemplate)
: mData(static_cast<unsigned char *>(malloc(aTemplate.mLength)))
, mLength(aTemplate.mLength)
, mVolume(aTemplate.mVolume)
{
	memcpy(mData, aTemplate.mData, mLength);
}

SoundTemplate::~SoundTemplate(void)
{
	free(mData);
}


Sound::Sound(void)
: Updatable(0)
, mData(NULL)
, mLength(0)
, mOffset(0)
, mVolume(0)
, mRepeat(0)
{
}

Sound::Sound(const SoundTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mData(aTemplate.mData)
, mLength(aTemplate.mLength)
, mOffset(0)
, mVolume(aTemplate.mVolume)
, mRepeat(0)
, mPosition(0, 0)
{
}

Sound::~Sound(void)
{
}

// hack!
extern Vector2 listenerpos;
void Sound::Update(float aStep)
{
	if (!mRepeat && mOffset >= mLength)
	{
		SDL_LockAudio();
		Database::sound.Delete(id);
		delete this;
		SDL_UnlockAudio();
		return;
	}

	if (Entity *entity = Database::entity.Get(id))
		mPosition = entity->GetPosition();
	else
		mPosition = listenerpos;
}


// AUDIO MIXER

static const int MAX_CHANNELS = 4;

static const float timestep = 1.0f / AUDIO_FREQUENCY;
static float average0 = 0.0f, average1 = 0.0f;
static const float averagefilter = 1.0f * timestep;
static const float minlevel = 32768.0f*32768.0f;
static float level = minlevel;
static const float levelfilter = 1.0f * timestep;
static const float postscale = 65534.0f / float(M_PI);

void mixaudio(void *userdata, Uint8 *stream, int len)
{
	int samples = len / sizeof(short);

	// if no sounds playing
	if (Database::sound.GetCount() == 0)
	{
		// update filters
		average0 -= average0 * 0.5f * samples * averagefilter;
		average1 -= average1 * 0.5f * samples * averagefilter;
		level -= level * 0.5f * samples * levelfilter;
		if (level < minlevel)
			level = minlevel;
		return;
	}

	// custom mixer
	static float mix[2048];
	memset(mix, 0, samples * sizeof(float));

	// listener position
	Vector2 &listenerpos = *static_cast<Vector2 *>(userdata);

	// sound channels
	unsigned char *channel_data[MAX_CHANNELS+1] = { 0 };
	unsigned int channel_offset[MAX_CHANNELS+1] = { 0 };
	unsigned int channel_length[MAX_CHANNELS+1] = { 0 };
	unsigned int channel_repeat[MAX_CHANNELS+1] = { 0 };
	float channel_volume[MAX_CHANNELS+1] = { 0 };
	float channel_weight[MAX_CHANNELS+1] = { 0 };
	int channel_count = 0;

	// for each active sound...
	for (Database::Typed<Sound *>::Iterator itor(&Database::sound); itor.IsValid(); ++itor)
	{
		// get the sound
		Sound *sound = const_cast<Sound *>(itor.GetValue());

		// apply sound fall-off
		float volume = sound->mVolume / (1.0f + listenerpos.DistSq(sound->mPosition) / 16384.0f);
		if (volume < 1.0f/256.0f)
			continue;

		// weight sound based on volume
		float weight = volume;

		// if not repeating...
		if (!sound->mRepeat)
		{
			// diminish weight over time
			weight *= (1.0f - sound->mOffset / sound->mLength);
		}

		// update channel data
		int j;
		for (j = channel_count - 1; j >= 0; j--)
		{
			if (channel_weight[j] >= weight) break;
			channel_data[j + 1] = channel_data[j];
			channel_offset[j + 1] = channel_offset[j];
			channel_length[j + 1] = channel_length[j];
			channel_repeat[j + 1] = channel_repeat[j];
			channel_volume[j + 1] = channel_volume[j];
			channel_weight[j + 1] = channel_weight[j];
		}
		if (j < MAX_CHANNELS - 1)
		{
			channel_data[j + 1] = sound->mData;
			channel_offset[j + 1] = sound->mOffset;
			channel_length[j + 1] = sound->mLength;
			channel_repeat[j + 1] = sound->mRepeat;
			channel_volume[j + 1] = volume;
			channel_weight[j + 1] = weight;
			if (channel_count < MAX_CHANNELS)
				++channel_count;
		}
	}

	// for each active channel...
	for (int channel = 0; channel < channel_count; ++channel)
	{
		// get starting offset
		unsigned char *data = channel_data[channel];
		unsigned int length = channel_length[channel];
		unsigned int offset = channel_offset[channel];
		float volume = channel_volume[channel];

		// while output to generate...
		int output = 0;
		while (output < samples)
		{
			// calculate amount to output
			int amount = std::min(size_t(length-offset)/sizeof(short), size_t(samples-output));

			// add volume-scaled sample
			for (int i = 0; i < amount; ++i)
				mix[output+i] += ((short *)(data+offset))[i] * volume;

			// advance offset
			offset += amount * sizeof(short);

			// if reaching the end...
			if (offset >= length)
			{
				// if repeating...
				if (channel_repeat[channel])
				{
					// loop around
					offset -= length;
				}
				else
				{
					// stop
					break;
				}
			}

			// advance output position
			output += amount;
		}
	}

	// for each active sound...
	for (Database::Typed<Sound *>::Iterator itor(&Database::sound); itor.IsValid(); ++itor)
	{
		// get the sound
		Sound *sound = const_cast<Sound *>(itor.GetValue());

		// update sound position
		sound->mOffset += len;

		// if moving past the end...
		if (sound->mOffset >= sound->mLength)
		{
			// if repeating
			if (sound->mRepeat)
			{
				// loop the sound
				sound->mOffset %= sound->mLength;
			}
			else
			{
				// clamp to the end
				sound->mOffset = sound->mLength;
			}
		}
	}

	// subract filtered average to remove DC term
	// apply filtered scaling to compress dynamic range
	// apply nonlinear curve to eliminate clipping
	float *src = mix;
	short *dst = (short *)stream;
	for (int i = 0; i < samples/2; ++i)
	{
		float mix0 = *src++;
		float mix1 = *src++;
		average0 += (mix0 -= average0) * averagefilter;
		average1 += (mix1 -= average1) * averagefilter;
		level += (mix0 * mix0 + mix1 * mix1 - level) * levelfilter;
		if (level < minlevel)
			level = minlevel;
		float prescale = InvSqrt(level);
		*dst++ = short(atanf(mix0 * prescale) * postscale);
		*dst++ = short(atanf(mix1 * prescale) * postscale);
	}
}

void PlaySound(unsigned int aId)
{
	const SoundTemplate &sound = Database::soundtemplate.Get(aId);
	if (sound.mData)
	{
		/* Put the sound data in the slot (it starts playing immediately) */
		SDL_LockAudio();
		Sound *s = new Sound(sound, aId);
		Database::sound.Put(aId, s);
		SDL_UnlockAudio();
	}
}
