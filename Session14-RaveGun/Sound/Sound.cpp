#include "StdAfx.h"
#include "Sound.h"
#include "Entity.h"

#include "Interpolator.h"

#include "ExpressionConfigure.h"


#if defined(USE_SDL_MIXER)

#include "SDL_Mixer.h"

#else

#define DISTANCE_FALLOFF(volume, distsq) ((volume) / (1.0f + (distsq) / 16384.0f))

#endif


// sound attributes
int SOUND_CHANNELS = 8;				// effect mixer channels
float SOUND_VOLUME_EFFECT = 0.5f;	// effects volume
float SOUND_VOLUME_MUSIC = 0.5f;	// music volume


// sound listener position
Vector2 Sound::listenerpos;


#ifdef USE_POOL_ALLOCATOR
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
	Typed<Typed<unsigned int> > soundcue(0xf23cbd5f /* "soundcue" */);
	Typed<Typed<Sound *> > sound(0x0e0d9594 /* "sound" */);
	Typed<std::string> musictemplate(0x19706bfe /* "musictemplate" */);
#ifdef USE_SDL_MIXER
	Typed<Mix_Music *> music(0x9f9c4fd4 /* "music" */);
#endif

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
				element->QueryIntAttribute("repeat", &sound.mRepeat);

				// if there are no sound cues...
				if (!Database::soundcue.Find(aId))
				{
					// add a default cue (HACK)
					Typed<unsigned int> &soundcue = Database::soundcue.Open(aId);
					soundcue.Put(0, aId);
					Database::soundcue.Close(aId);
				}

				// clear sound data
				sound.mData = NULL;
				sound.mSize = 0;
				sound.mLength = 0;

				// configure the sound template
				sound.Configure(element, aId);

				// close sound template
				Database::soundtemplate.Close(aId);
			}
		}
		soundloader;

		class SoundCueLoader
		{
		public:
			SoundCueLoader()
			{
				AddConfigure(0xf23cbd5f /* "soundcue" */, Entry(this, &SoundCueLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// open soundcue
				Typed<unsigned int> &soundcue = Database::soundcue.Open(aId);

				// if the object has an embedded sound...
				if (Database::soundtemplate.Find(aId))
				{
					// remove the default cue (HACK)
					soundcue.Delete(0);
				}

				// process sound cue configuration
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0xe5561300 /* "cue" */:
						{
							// assign cue
							unsigned int subid = Hash(child->Attribute("name"));
							unsigned int &cue = soundcue.Open(subid);
							cue = Hash(child->Attribute("sound"));
							soundcue.Close(subid);
						}
						break;
					}
				}

				// close soundcue
				Database::soundcue.Close(aId);
			}
		}
		soundcueloader;

		class MusicLoader
		{
		public:
			MusicLoader()
			{
				AddConfigure(0x9f9c4fd4 /* "music" */, Entry(this, &MusicLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				if (const char *file = element->Attribute("file"))
					musictemplate.Put(aId, file);
			}
		}
		musicloader;
	}

	namespace Initializer
	{
		class SoundInitializer
		{
		public:
			SoundInitializer()
			{
				AddActivate(0xf23cbd5f /* "soundcue" */, Entry(this, &SoundInitializer::Activate));
				AddPostActivate(0xf23cbd5f /* "soundcue" */, Entry(this, &SoundInitializer::PostActivate));
				AddDeactivate(0xf23cbd5f /* "soundcue" */, Entry(this, &SoundInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
#ifdef SOUND_VALIDATE_CUE
				for (Typed<unsigned int>::Iterator itor(soundcue.Find(aId)); itor.IsValid(); ++itor)
				{
					if (const SoundTemplate *sound = soundtemplate.Find(itor.GetValue()))
					{
						if (sound->mLength == 0)
						{
							DebugPrint("Entity %s cue %s empty sound %s\n", Database::name.Get(aId).c_str(), Database::name.Get(itor.GetKey()).c_str(), Database::name.Get(itor.GetValue()).c_str());
						}
					}
					else
					{
						DebugPrint("Entity %s cue %s missing sound %s\n", Database::name.Get(aId).c_str(), Database::name.Get(itor.GetKey()).c_str(), Database::name.Get(itor.GetValue()).c_str());
					}
				}
#endif
			}

			void PostActivate(unsigned int aId)
			{
				PlaySoundCue(aId, 0);
			}

			void Deactivate(unsigned int aId)
			{
#if defined(USE_SDL)
				if (Database::sound.Find(aId))
				{
					SDL_LockAudio();
					const Typed<Sound *> &sounds = Database::sound.Get(aId);
					for (Typed<Sound *>::Iterator itor(&sounds); itor.IsValid(); ++itor)
						delete itor.GetValue();
					Database::sound.Delete(aId);
					SDL_UnlockAudio();
				}
#endif
			}
		}
		soundinitializer;

#if defined(USE_SDL_MIXER)
		class MusicInitializer
		{
		public:
			MusicInitializer()
			{
				AddActivate(0x19706bfe /* "musictemplate" */, Entry(this, &MusicInitializer::Activate));
				AddDeactivate(0x19706bfe /* "musictemplate" */, Entry(this, &MusicInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				// HACK
				const std::string &music = Database::musictemplate.Get(aId);
				Mix_Music *musicdata = Mix_LoadMUS(music.c_str());
				if (!musicdata)
					DebugPrint("%s\n", Mix_GetError());
				if (Mix_PlayMusic(musicdata, -1) < 0)
					DebugPrint("%s\n", Mix_GetError());
				Database::music.Put(aId, musicdata);
			}

			void Deactivate(unsigned int aId)
			{
				if (Mix_Music *musicdata = Database::music.Get(aId))
				{
					Mix_HaltMusic();
					Mix_FreeMusic(musicdata);
					Database::music.Delete(aId);
				}
			}
		}
		musicinitializer;
#endif
	}
}

SoundTemplate::SoundTemplate(void)
: mData(NULL)
, mSize(0)
, mLength(0)
, mVolume(1.0f)
, mRepeat(0)
{
}

SoundTemplate::SoundTemplate(const SoundTemplate &aTemplate)
: mData(malloc(aTemplate.mSize))
, mSize(aTemplate.mSize)
, mLength(aTemplate.mLength)
, mVolume(aTemplate.mVolume)
, mRepeat(aTemplate.mRepeat)
{
	memcpy(mData, aTemplate.mData, mSize);
}

SoundTemplate::~SoundTemplate(void)
{
	free(mData);
}

typedef bool (*ApplyInterpolatorFunc)(float target[], int width, int count, const float keys[], float aTime, int &aIndex);

static bool ApplyConstant(float target[], int width, int count, const float keys[], float aTime, int &aIndex)
{
	memcpy(&target[0], &keys[1], width * sizeof(target[0]));
	return true;
}

bool SoundTemplate::ConfigureFile(const TiXmlElement *element, unsigned int id)
{
	const char *name = element->Attribute("name");
	if (name == NULL)
		return false;

#if defined(USE_SDL_MIXER)
	// load sound file
	Mix_Chunk *loadchunk = Mix_LoadWAV(name);
	if (!loadchunk)
	{
		DebugPrint("error loading sound file \"%s\": %s\n", name, Mix_GetError());
		return false;
	}

	// copy sound data
	mSize = mLength * sizeof(short) + loadchunk->alen;
	mData = realloc(mData, mSize);
	memcpy(static_cast<short *>(mData) + mLength, loadchunk->abuf, loadchunk->alen);
	mLength = mSize / sizeof(short);

	// free loaded data
	Mix_FreeChunk(loadchunk);

#elif defined(USE_SDL)
	// load wave file data
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	if ( !SDL_LoadWAV(name, &wave, &data, &dlen) )
	{
		DebugPrint("error loading sound file \"%s\": %s\n", name, SDL_GetError());
		return false;
	}

	// build audio conversion
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq,
							AUDIO_S16,   1,             AUDIO_FREQUENCY);

	// append sound data
	mSize = mLength * sizeof(short) + dlen * cvt.len_mult;
	mData = realloc(mData, mSize);
	memcpy(static_cast<short *>(mData) + mLength, data, dlen);
	cvt.buf = reinterpret_cast<unsigned char *>(static_cast<short *>(mData) + mLength);
	cvt.len = dlen;

	// convert to final format
	SDL_ConvertAudio(&cvt);
	mLength += cvt.len_cvt / sizeof(short);
	mSize = mLength * sizeof(short);
	mData = realloc(mData, mSize);

	// release wave file data
	SDL_FreeWAV(data);
#endif

	return true;
}

/*
bool SoundTemplate::ConfigureSynth(const TiXmlElement *element, unsigned int id)
{
	// sample length
	float length = 0;
	element->QueryFloatAttribute("length", &length);
	int samples = xs_CeilToInt(length * AUDIO_FREQUENCY);

	// oversampling
	int oversample = 4;
	element->QueryIntAttribute("oversample", &oversample);

	// append sound data
	mSize = (mLength + samples) * sizeof(short);
	mData = realloc(mData, mSize);

	// initialize timer
	float time = 0.0f;
	const float steptime = 1.0f / float(AUDIO_FREQUENCY * oversample);

	// clock frequency
	int frequency = 1;
	element->QueryIntAttribute("frequency", &frequency);

	// period (in clock units)
	float period = 1.0f;
	element->QueryFloatAttribute("period", &period);

	// amplitude
	float amplitude = 1.0f;
	element->QueryFloatAttribute("amplitude", &amplitude);

	// offset
	float offset = 0.0f;
	element->QueryFloatAttribute("offset", &offset);

	// process data stream
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
	}
}
*/

bool SoundTemplate::ConfigureSample(const TiXmlElement *element, unsigned int id)
{
	// sample length
	float length = 0;
	element->QueryFloatAttribute("length", &length);
	int samples = xs_CeilToInt(length * AUDIO_FREQUENCY);

	// append sound data
	mSize = (mLength + samples) * sizeof(short);
	mData = realloc(mData, mSize);

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
		short sample = short(Clamp(xs_RoundToInt(accum * 32767.0f / oversample), SHRT_MIN, SHRT_MAX));
		static_cast<short *>(mData)[mLength++] = sample;
	}

	return true;
}

class Pokey
{
public:
	bool poly4[(1<<4) - 1];
	bool poly5[(1<<5) - 1];
	bool poly9[(1<<9) - 1];
	bool poly17[(1<<17) - 1];

public:
	Pokey()
	{
#define POKEY_TYPE_MAME 0
#define POKEY_TYPE_ATARI800 1
#define POKEY_TYPE_GALOIS 2
#define POKEY_TYPE 1

#if POKEY_TYPE == POKEY_TYPE_MAME

		InitPoly(poly4,   4,  3, 1, 0x00004);
		//Poly4:
		//000111101011001|15

		InitPoly(poly5,   5,  3, 2, 0x00008);
		//Poly5:
		//0000011100100010101111011010011|31

		InitPoly(poly9,   9,  8, 1, 0x00180);
		//Poly9:
		//0000000010101010110011001110111011110000111110101111110011111110111111110|73
		//0000000010101010110011001110111011110000111110101111110011111110111111110|146
		//0000000010101010110011001110111011110000111110101111110011111110111111110|219
		//0000000010101010110011001110111011110000111110101111110011111110111111110|292
		//0000000010101010110011001110111011110000111110101111110011111110111111110|365
		//0000000010101010110011001110111011110000111110101111110011111110111111110|438
		//0000000010101010110011001110111011110000111110101111110011111110111111110|511

		InitPoly(poly17, 17, 16, 1, 0x1C000);
		//Poly17:
		//00000000000000010010010010010010100010100010100011001101001101001110000110000110000010011100101100100010111000111000100011110011
		//11001100111110000001011000000010010001110010010100010000010100011001100100011001110000101001110000010011010000010010100001100100
		//01010100101100010001101000111001100111010011110000101110100000010011011101001001100001111010001011001011110100110110001111101000
		//01110011111101001011100000000110001111001001000010000001010001001100100110100110000101000011000010011010010110010100001100011100
		//01100101100111100111000111000000101110011110010011011100000010100001111001001101001011110001011010001111100110110100111111000011
		//10100000000100111101001001000101111010001010011011110100110100001111101000011001011111010010110001111110100011100111111101001111
		//00000000011000000100100100001001000101000100110001000110011000010001000010110010100110010101100011000010101011001110010101010110
		//00001010101010110010001101010101100010000110101011001100101101010110000101011010101100101010110101011000110101101010110011101011|33383|66766|100149

#elif POKEY_TYPE == POKEY_TYPE_ATARI800

		InitPoly(poly4, 4, 1, 0xF, false);
		//Poly4 tap=1 seed=0000000f invert=0
		//111100010011010|15

		InitPoly(poly5, 5, 2, 0x1F, true);
		//Poly5 tap=2 seed=0000001f invert=1
		//0000011100100010101111011010011|31

		InitPoly(poly9, 9, 5, 0x1FF, false);
		//Poly9 tap=5 seed=000001ff invert=0
		//11111111100001111011100001011001101101111010000111001100001001000101011101011110010010111001110000001110111010011110101001010000
		//00101010101111101011010000011011101101101011000001011101111100011110011010011010111000110100010111111101001011000101001100011000
		//00001100110010101100100111111011010010010011011111100101101010000101000100111011001011110110000110101010011100100001100010000100
		//0000001000100011001000111010101101100011100010010101000110110011111001111000101101110010100100000100110011101000111110111100000|511

		InitPoly(poly17, 17, 5, 0x1FFFF, false);
		//Poly17 tap=5 seed=0001ffff invert=0
		//11111111111111111000000000000111110000000111111111100111110000011000111111111000001110000111111110011011110001100111000110000100
		//00100000100000000001100001000001100010000011100001000111100110000101101001001001100010011011110001011111000110111101111000110000
		//00001100000110001100011110000000000110111100000110110001111110111100011101000001100101111011100110110000010001111101110001011110
		//00101011110011011010100100111100010001110101101001010110000010000100101110000000100111011100100110010010011111011110111011100000
		//00011001011100011001101110100001011101111100011111000111100111111001101000110110011011100111110111010000111000111110011001001111
		//10010111110111101011011000000100001110110100000110101001101110100100111101111001110100000010000111101010000011100101010111100010
		//01110100110101100111011100001100010010110100001010010001100010000101000001000001001011000011001001010101001111000111100101011001
		//10101001001101100100011111111111010111000000101001101110101000111101110101101100001110000111010110011011011000110111101110000110|131071

#elif POKEY_TYPE == POKEY_TYPE_GALOIS

		InitPoly(poly4, 4, (1<<3)|(1<<2), 0xF, false);
		//LFSR4 mask=0000000c seed=0000000f invert=0
		//111000100110101|15

		InitPoly(poly5, 5, (1<<4)|(1<<2), 0x1F, true);
		//LFSR5 mask=00000014 seed=0000001f invert=1
		//0001110010001010111101101001100|31

		InitPoly(poly9, 9, (1<<8)|(1<<3), 0x1FF, false);
		//LFSR9 mask=00000108 seed=000001ff invert=0
		//11110000111101110000101100110110111101000011100110000100100010101110101111001001011100111000000111011101001111010100101000000101
		//01010111110101101000001101110110110101100000101110111110001111001101001101011100011010001011111110100101100010100110001100000001
		//10011001010110010011111101101001001001101111110010110101000010100010011101100101111011000011010101001110010000110001000010000000
		//0100010001100100011101010110110001110001001010100011011001111100111100010110111001010010000010011001110100011111011110000011111|511

		InitPoly(poly17, 17, (1<<16)|(1<<11), 0x1FFFF, false);
		//LFSR17 mask=00010800 seed=0001ffff invert=0
		//11111111111100000000000011111000000011111111110011111000001100011111111100000111000011111111001101111000110011100011000010000100
		//00010000000000110000100000110001000001110000100011110011000010110100100100110001001101111000101111100011011110111100011000000001
		//10000011000110001111000000000011011110000011011000111111011110001110100000110010111101110011011000001000111110111000101111000101
		//01111001101101010010011110001000111010110100101011000001000010010111000000010011101110010011001001001111101111011101110000000011
		//00101110001100110111010000101110111110001111100011110011111100110100011011001101110011111011101000011100011111001100100111110010
		//11111011110101101100000010000111011010000011010100110111010010011110111100111010000001000011110101000001110010101011110001001110
		//10011010110011101110000110001001011010000101001000110001000010100000100000100101100001100100101010100111100011110010101100110101
		//00100110110010001111111111101011100000010100110111010100011110111010110110000111000011101011001101101100011011110111000011000000|131071

#else
#error "Undefine POKEY type"
#endif
/*
		Polynomials for Maximum LFSR:
		http://homepage.mac.com/afj/taplist.html

		Stargate 16-bit LFSR
		seed: 0x3C00
		output = random & 1
		random = ((((random >> 3) ^ (random)) & 1) << 15) | (random >> 1);
		L=57337: 00000000001111000000000111011100000011110011110001110110110111111100000000100001110000010010111111001000010100011000101011011101
*/
	}

#if POKEY_TYPE == POKEY_TYPE_MAME
	// from MAME pokey.c
	size_t InitPoly(bool aOut[], int aSize, int aLeft, int aRight, int aAdd)
	{
		DebugPrint("Poly%d:", aSize);
		int mask = (1 << aSize) - 1;
		int x = 0;
		int repeat = 0;
		for (int i = 0; i < mask; ++i)
		{
			if (i < 1024)
			{
				if (((i-repeat)&127)==0)
					DebugPrint("\n");
				DebugPrint(x&1?"1":"0");
			}
			aOut[i] = (x & 1) != 0;
			x = ((x << aLeft) + (x >> aRight) + aAdd) & mask;
			if (x == 0)
			{
				DebugPrint("|%d", i+1);
				repeat = i+1;
			}
		}
		DebugPrint("\n");
		return mask;
	}
#elif POKEY_TYPE == POKEY_TYPE_ATARI800
	// from Atari800 pokey.c
	size_t InitPoly(bool aOut[], int aSize, int aTap, unsigned int aSeed, bool aInvert = false)
	{
		DebugPrint("Poly%d tap=%d seed=%08x invert=%d", aSize, aTap, aSeed, aInvert);
		unsigned int x = aSeed;
		unsigned int i = 0;
		do
		{
			aOut[i] = (x & 1) ^ aInvert;
			if (i < 1024)
			{
				if ((i&127)==0)
					DebugPrint("\n");
				DebugPrint(aOut[i]?"1":"0");
			}
			x = ((((x >> aTap) ^ x) & 1) << (aSize - 1)) | (x >> 1);
			++i;
		}
		while (x != aSeed);
		DebugPrint("|%d\n", i);
		return i;
	}
#elif POKEY_TYPE == POKEY_TYPE_GALOIS
	// from http://en.wikipedia.org/wiki/Linear_feedback_shift_register
	size_t InitPoly(bool aOut[], int aSize, unsigned int aMask, unsigned int aSeed = 0, bool aInvert = false)
	{
		DebugPrint("LFSR%d mask=%08x seed=%08x invert=%d", aSize, aMask, aSeed, aInvert);
		unsigned int x = aSeed;
		unsigned int i = 0;
		do
		{
			aOut[i] = (x & 1) ^ aInvert;
			if (i < 1024)
			{
				if ((i&127)==0)
					DebugPrint("\n");
				DebugPrint(aOut[i]?"1":"0");
			}
			x = (x>>1) ^ (-int(x&1)&aMask);
			++i;
		}
		while (x != aSeed);
		DebugPrint("|%d\n", i);
		return i;
	}
#else
#error "Undefined POKEY type"
#endif
}
pokey;

bool SoundTemplate::ConfigurePokey(const TiXmlElement *element, unsigned int id)
{
	// sample length
	float length = 0;
	element->QueryFloatAttribute("length", &length);
	int samples = xs_CeilToInt(length * AUDIO_FREQUENCY);

	// append sound data
	mSize = (mLength + samples) * sizeof(short);
	mData = realloc(mData, mSize);

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

	// keyframe data
	std::vector<unsigned int> dividerkey;
	float dividerquant = 1;
	std::vector<unsigned int> amplitudekey;
	float amplitudequant = 1.0f/65536.0f;
	std::vector<unsigned int> offsetkey;
	float offsetquant = 1.0f/65536.0f;

	const char *names[1] = { "value" };

	ApplyInterpolatorFunc dividerfunc = ApplyConstant;
	ApplyInterpolatorFunc amplitudefunc = ApplyConstant;
	ApplyInterpolatorFunc offsetfunc = ApplyConstant;

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x36b04926 /* "divider" */:
			{
				child->QueryFloatAttribute("value", &divider);
				child->QueryFloatAttribute("quantize", &dividerquant);
				if (ConfigureInterpolatorItem(child, dividerkey, 1, names, &divider))
				{
					int interpolate = 1;
					child->QueryIntAttribute("interpolate", &interpolate);
					dividerfunc = interpolate ? ApplyInterpolator : ApplyInterpolatorConstant;
				}
			}
			break;

		case 0x16746aa2 /* "amplitude" */:
			{
				child->QueryFloatAttribute("value", &amplitude);
				child->QueryFloatAttribute("quantize", &amplitudequant);
				int interpolate = 1;
				child->QueryIntAttribute("interpolate", &interpolate);
				amplitudefunc = interpolate ? ApplyInterpolator : ApplyInterpolatorConstant;
				ConfigureInterpolatorItem(child, amplitudekey, 1, names, &amplitude);
			}
			break;

		case 0x14c8d3ca /* "offset" */:
			{
				child->QueryFloatAttribute("value", &offset);
				child->QueryFloatAttribute("quantize", &offsetquant);
				int interpolate = 1;
				child->QueryIntAttribute("interpolate", &interpolate);
				offsetfunc = interpolate ? ApplyInterpolator : ApplyInterpolatorConstant;
				ConfigureInterpolatorItem(child, offsetkey, 1, names, &offset);
			}
			break;

		}
	}

	if (dividerkey.empty())
	{
		dividerkey.push_back(1);
		dividerkey.push_back(0);
		dividerkey.push_back(*reinterpret_cast<unsigned int *>(&divider));
		dividerfunc = ApplyConstant;
	}
	if (amplitudekey.empty())
	{
		amplitudekey.push_back(1);
		amplitudekey.push_back(0);
		amplitudekey.push_back(*reinterpret_cast<unsigned int *>(&amplitude));
		amplitudefunc = ApplyConstant;
	}
	if (offsetkey.empty())
	{
		offsetkey.push_back(1);
		offsetkey.push_back(0);
		offsetkey.push_back(*reinterpret_cast<unsigned int *>(&offset));
		offsetfunc = ApplyConstant;
	}

	bool *poly1data = NULL;
	int poly1size = 1;
	bool *poly2data = NULL;
	int poly2size = 1;

	switch (Hash(element->Attribute("tone")))
	{
	case 0x966dd8e3 /* "pure" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = NULL;
		poly2size = 1;
		break;

	case 0x90ecc009 /* "poly4" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = pokey.poly4;
		poly2size = SDL_arraysize(pokey.poly4);
		break;

	case 0x9becd15a /* "poly9" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = pokey.poly9;
		poly2size = SDL_arraysize(pokey.poly9);
		break;

	case 0xd2ba0daf /* "poly17" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = pokey.poly17;
		poly2size = SDL_arraysize(pokey.poly17);
		break;

	case 0x8fecbe76 /* "poly5" */:
		poly1data = pokey.poly5;
		poly1size = SDL_arraysize(pokey.poly5);
		poly2data = NULL;
		poly2size = 1;
		break;

	case 0x3746e7fb /* "poly5+poly4" */:
		poly1data = pokey.poly5;
		poly1size = SDL_arraysize(pokey.poly5);
		poly2data = pokey.poly4;
		poly2size = SDL_arraysize(pokey.poly4);
		break;

	case 0x3246e01c /* "poly5+poly9" */:
		poly1data = pokey.poly5;
		poly1size = SDL_arraysize(pokey.poly5);
		poly2data = pokey.poly9;
		poly2size = SDL_arraysize(pokey.poly9);
		break;

	case 0x40a65239 /* "poly5+poly17" */:
		poly1data = pokey.poly5;
		poly1size = SDL_arraysize(pokey.poly5);
		poly2data = pokey.poly17;
		poly2size = SDL_arraysize(pokey.poly17);
		break;

	default:
		break;
	}

	const int oversample = 4;
	const float stepticks = float(frequency) / float(AUDIO_FREQUENCY * oversample);

	float counter = 0;
	int poly1index = xs_FloorToInt(mLength * oversample * stepticks) % poly1size;
	int poly2index = xs_FloorToInt(mLength * oversample * stepticks) % poly2size;
	bool outputhigh = true;

	float time = 0.0f;
	const float steptime = 1.0f / float(AUDIO_FREQUENCY * oversample);
	int dividerhint = 0;
	int amplitudehint = 0;
	int offsethint = 0;

	// for each sample...
	for (int i = 0; i < samples; ++i)
	{
		// for each oversample...
		float accum = 0;
		for (int j = 0; j < oversample; ++j)
		{
			// get current divider value
			float divider;
			dividerfunc(&divider, 1, dividerkey[0], reinterpret_cast<const float * __restrict>(&dividerkey[1]), time, dividerhint);
			divider = xs_RoundToInt(divider / dividerquant) * dividerquant;

			// if the counter reaches the divider...
			if (counter >= divider)
			{
				// update the counter
				counter -= divider;

				// perform one update tick
				if ((!poly1data) ||
					(poly1data[poly1index = xs_FloorToInt(poly1index + divider) % poly1size]))
				{
					if (poly2data)
						outputhigh = poly2data[poly2index = xs_FloorToInt(poly2index + divider) % poly2size];
					else
						outputhigh = !outputhigh;
				}
			}

			// get current amplitude value
			float amplitude;
			amplitudefunc(&amplitude, 1, amplitudekey[0], reinterpret_cast<const float * __restrict>(&amplitudekey[1]), time, amplitudehint);
			amplitude = xs_RoundToInt(amplitude / amplitudequant) * amplitudequant;

			// get current offset value
			float offset;
			offsetfunc(&offset, 1, offsetkey[0], reinterpret_cast<const float * __restrict>(&offsetkey[1]), time, offsethint);
			offset = xs_RoundToInt(offset / offsetquant) * offsetquant;

			// accumulate value
			accum += offset + (outputhigh ? amplitude : -amplitude);

			// advance the counter
			counter += stepticks;

			// advance time
			time += steptime;
		}

		// append sample
		short sample = short(Clamp(xs_RoundToInt(accum * 32767.0f / oversample), SHRT_MIN, SHRT_MAX));
		static_cast<short *>(mData)[mLength++] = sample;
	}

	return true;
}

static float OutputPulse(SoundTemplate *self, int ticks, float samplespertick, float samples, short sample)
{
	samples += ticks * samplespertick;
	int count = xs_FloorToInt(samples);
	if (count > 0)
	{
		samples -= count;

		// round up
		size_t newsize = ((self->mLength + count) * sizeof(short) + 255) & ~255;
		if (self->mSize < newsize)
		{
			self->mSize = newsize;
			self->mData = realloc(self->mData, self->mSize);
		}
		for (int i = 0; i < count; ++i)
		{
			static_cast<short *>(self->mData)[self->mLength++] = sample;
		}
	}
	return samples;
}


// linear feedback shift register
static unsigned short random = 0x3C00;

static bool ConfigureTriangleNoise(SoundTemplate *self, const TiXmlElement *element, unsigned int id)
{
	// linear feedback shift register
	unsigned short baseslope = 0;		// [0x13,0x15]
	unsigned short duration = 0;		// [0x16,0x17]
	unsigned char decay = 0;			// [0x18]
	unsigned char sputter = 0;			// [0x19]
	unsigned char output = 0;			// [0x400]

	// clock frequency
	int frequency = 0;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 1;
	element->QueryFloatAttribute("divider", &divider);

	// outer loop delay ticks
	int outerdelay = xs_FloorToInt(frequency/(divider*AUDIO_FREQUENCY));
	element->QueryIntAttribute("outerdelay", &outerdelay);

	// inner loop delay ticks
	int innerdelay = xs_FloorToInt(frequency/(divider*AUDIO_FREQUENCY));
	element->QueryIntAttribute("innerdelay", &innerdelay);

	int value;

	// base slope
	if (element->QueryIntAttribute("slope", &value) == TIXML_SUCCESS)
		baseslope = unsigned short(value << 8);

	// duration
	if (element->QueryIntAttribute("duration", &value) == TIXML_SUCCESS)
		duration = unsigned short(value);

	// decay
	if (element->QueryIntAttribute("decay", &value) == TIXML_SUCCESS)
		decay = value != 0;

	// sputter
	if (element->QueryIntAttribute("sputter", &value) == TIXML_SUCCESS)
		sputter = value != 0;

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;

	/// sound generator loop F930
	do
	{
		// loop counter
		unsigned short X = duration;

		// current value 8:8
		unsigned short value = output << 8;
		do
		{
			// update linear feedback shift register
			// random = ((((random >> 3) ^ random) & 1) << 15) | (random >> 1);
			// not sure what difference using the value makes,
			// but that's what the original M6808 code does
			random = ((((value >> 11) ^ random) & 1) << 15) | (random >> 1);

			// get slope 8:8 for this iteration
			unsigned short curslope = baseslope;

			// if applying sputter...
			if (sputter)
			{
				// randomize the slope
				curslope &= random | 0xFF;
			}

			// target value 8:8
			unsigned short target = (random << 8) & 0xFFFF;

			// if the current value is less than or equal to the target value...
			if (value <= target)
			{
				// ramp up towards the target
				do
				{
					// count down duration
					--X;
					if (X == 0)
						goto F985;

					// output the current value
					output = value >> 8;
					nextticks += innerdelay;
					samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
					prevticks = nextticks;

					// get the new value
					unsigned int newvalue = unsigned int(value) + unsigned int(curslope);

					// stop if the value overshot and wrapped around
					if (newvalue > 0xFFFF)
						break;

					// update the current value
					value = newvalue & 0xFFFF;
				}
				while (value <= target);
			}
			else
			{
				// ramp down towards the target
				do
				{
					// count down duration
					--X;
					if (X == 0)
						goto F985;

					// output the current value
					output = value >> 8;
					nextticks += innerdelay;
					samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
					prevticks = nextticks;

					// get the new value
					unsigned int newvalue = unsigned int(value) - unsigned int(curslope);

					// stop if the value overshot and wrapped around
					if (newvalue > 0xFFFF)
						break;

					// update the current value
					value = newvalue & 0xFFFF;
				}
				while (value > target);
			}

			// snap to target
			value = target;

			// add ticks
			nextticks += outerdelay;

			// output the current value
			output = value >> 8;
			samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
			prevticks = nextticks;
		}
		while(true);

F985:
		// if applying decay
		if (decay)
		{
			// scale slope by 7/8
			baseslope -= (baseslope >> 3);
			nextticks += 4*2+2*8+4*3+5*2+4;
		}
		else
		{
			// mark as repeating
			self->mRepeat = true;
			break;
		}
	}
	while (baseslope > 0x0007);

	// ramp to zero to prevent popping
	while (output != 0)
	{
		--output;
		nextticks += 8*4*256/baseslope;
		samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
		prevticks = nextticks;
	}

	return true;
}

static bool ConfigureBitNoiseRamp(SoundTemplate *self, const TiXmlElement *element, unsigned int id)
{
	// clock frequency
	int frequency = 0;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 1;
	element->QueryFloatAttribute("divider", &divider);

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;

	/*
	// sound 0x15	; enemy warp in 
	unsigned char delaydelta = 0xFE;	// [0x1A]
	unsigned char delay = 0xC0;			// A
	unsigned char repeat = 0x10;		// B
	*/

	int outersteps = 0;
	element->QueryIntAttribute("outersteps", &outersteps);

	int outerdelay = 0;
	element->QueryIntAttribute("outerdelay", &outerdelay);

	int innersteps = 0;
	element->QueryIntAttribute("innersteps", &innersteps);

	int innerdelay = 0;
	element->QueryIntAttribute("innerdelay", &innerdelay);

	int innerdelta = 0;
	element->QueryIntAttribute("innerdelta", &innerdelta);

	int amplitude = 255;
	element->QueryIntAttribute("amplitude", &amplitude);

	int amplitudedelta = 0;
	element->QueryIntAttribute("amplitudedelta", &amplitudedelta);

	// sound generator loop F89E
	unsigned char output = 0xFF;

	// for each outer step...
	for (int outer = outersteps; outer > 0; --outer)
	{
		// for each inner step
		for (int inner = innersteps; inner > 0; --inner)
		{
			// if a bit shifts out...
			if (random & 1)
			{
				// output samples
				samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
				prevticks = nextticks;

				// flip output
				output = output ? 0 : unsigned char(amplitude);
			}

			// update shift register
			random = ((((random >> 3) ^ (random)) & 1) << 15) | (random >> 1);

			// update tick count
			nextticks += innerdelay;
		}

		// update tick count
		nextticks += outerdelay;

		// update delay
		innerdelay += innerdelta;

		// update amplitude
		amplitude += amplitudedelta;
	}

	// output samples
	samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);

	return true;
}


// "Big Red" fire sound
static bool ConfigureStargateF9A6(SoundTemplate *self, const TiXmlElement *element, unsigned int id)
{
	// clock frequency
	int frequency = 3579000;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 4;
	element->QueryFloatAttribute("divider", &divider);

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;

	// wave data
	unsigned char data[16] =
	{
		0x8C, 0x5B, 0xB6, 0x40, 0xBF, 0x49, 0xA4, 0x73,
		0x73, 0xA4, 0x49, 0xBF, 0x40, 0xB6, 0x5B, 0x8C,
	};

	unsigned char mem_0x11 = 0x00;
	unsigned short mem_0x0B_0x0C = 0x0064;
	unsigned char A = 0xFD, B = 0x00;
	unsigned short X;

	do
	{
		A = mem_0x11;
		int value = (A << 8) + B + mem_0x0B_0x0C;
		A = (value >> 8) & 0xFF;
		B = value & 0xFF;
		mem_0x11 = A;
		X = mem_0x0B_0x0C;
		if (value > 0xFFFF)
		{
			++X;
			nextticks += 4+4;
			if (X == 0x0070)
				break;
		}
		mem_0x0B_0x0C = X;
		nextticks += 4+4+4+5+4+4+4+5+4+4+5+5+4+5+4;
		OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * data[A & 0x0F]);
		prevticks = nextticks;
	}
	while(true);

	return true;
}


// 
static bool ConfigurePulseLoop(SoundTemplate *self, const TiXmlElement *element, unsigned int id)
{
	// clock frequency
	int frequency = 3579000;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 4;
	element->QueryFloatAttribute("divider", &divider);

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;

	/*
	;;; sound parameters
	A=00 (sound 0x1C):	FD3C	40 01 00 10 E1 0080 FF FF	; ??? enforcer fire
	A=01 (sound 0x1D):	FD45	28 01 00 08 81 0200 FF FF	; extra life
	A=02 (sound 0x1E):	FD4E	28 81 00 FC 01 0200 FC FF	; startup
	A=03 (sound 0x1F):	FD57	FF 01 00 18 41 0480 00 FF	; ???

	;;; arguments
	; 0x13: first pulse loop count at start of middle wave loop
	; 0x14: second pulse loop count at start of middle wave loop
	; 0x15: first pulse loop delta per iteration of middle wave loop
	; 0x16: second pulse loop delta per iteration of middle wave loop
	; 0x17: end middle wave loop second pulse loop count equals this
	; 0x18,0x19: inner wave loop total length
	; 0x1A: first pulse loop delta per iteration of outer wave loop
	; 0x1B: starting output value

	;;; locals
	; 0x1C: first pulse loop counter
	; 0x1D: second pulse loop counter
	*/

	int value;

	unsigned char pulse1delay = 0;			// [0x13]
	if (element->QueryIntAttribute("pulse1delay", &value) == TIXML_SUCCESS)
		pulse1delay = unsigned char(value);

	unsigned char pulse2delay = 0;			// [0x14]
	if (element->QueryIntAttribute("pulse2delay", &value) == TIXML_SUCCESS)
		pulse2delay = unsigned char(value);

	unsigned char pulse1innerdelta = 0;		// [0x15]
	if (element->QueryIntAttribute("pulse1innerdelta", &value) == TIXML_SUCCESS)
		pulse1innerdelta = unsigned char(value);

	unsigned char pulse2innerdelta = 0;		// [0x16]
	if (element->QueryIntAttribute("pulse2innerdelta", &value) == TIXML_SUCCESS)
		pulse2innerdelta = unsigned char(value);

	unsigned char pulse1outerdelta = 0;		// [0x1A]
	if (element->QueryIntAttribute("pulse1outerdelta", &value) == TIXML_SUCCESS)
		pulse1outerdelta = unsigned char(value);

	unsigned char pulse2outerdelta = 0;		// new!
	if (element->QueryIntAttribute("pulse2outerdelta", &value) == TIXML_SUCCESS)
		pulse2outerdelta = unsigned char(value);

	unsigned char pulse1output = 0;				// ~[0x1B]
	if (element->QueryIntAttribute("pulse1output", &value) == TIXML_SUCCESS)
		pulse1output = unsigned char(value);

	unsigned char pulse2output = 0;				// [0x1B]
	if (element->QueryIntAttribute("pulse2output", &value) == TIXML_SUCCESS)
		pulse2output = unsigned char(value);

	int totalpulsedelay = 0;				// [0x18,0x19]
	element->QueryIntAttribute("totalpulsedelay", &totalpulsedelay);

	int innercount = 0;						// smallest value such that ([0x14] + [0x16] * innercount) % 256 == [0x17]
	element->QueryIntAttribute("innercount", &innercount);

	int outercount = 0;						// smallest value such that ([0x13] + [0x1A] * outercount) % 256 == 0
	element->QueryIntAttribute("outercount", &outercount);

	int outerdelaypre = 0;
	element->QueryIntAttribute("outerdelaypre", &outerdelaypre);

	int innerdelaypre = 0;
	element->QueryIntAttribute("innerdelaypre", &innerdelaypre);

	int pulsedelaybase = 0;
	element->QueryIntAttribute("pulsedelaybase", &pulsedelaybase);

	int pulsedelayscale = 1;
	element->QueryIntAttribute("pulsedelayscale", &pulsedelayscale);

	int innerdelaypost = 0;
	element->QueryIntAttribute("innerdelaypost", &innerdelaypost);

	int outerdelaypost =0;
	element->QueryIntAttribute("outerdelaypost", &outerdelaypost);


	// sound generator loop

	// outer repeat loop
	for (int outer = outercount; outer > 0; --outer)
	{
		unsigned char pulse1 = pulse1delay;
		unsigned char pulse2 = pulse2delay;

		nextticks += outerdelaypre;
		OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
		prevticks = nextticks;

		// inner repeat loop
		for (int inner = innercount; inner > 0; --inner)
		{
			int delaylimit = totalpulsedelay;

			nextticks += innerdelaypre;
			OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
			prevticks = nextticks;

			// pulse loop
			do	
			{
				int steps;
				
				// pulse 1
				steps = std::min<int>(pulse1, delaylimit);
				nextticks += pulsedelaybase + pulsedelayscale * steps;
				OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse1output);
				prevticks = nextticks;

				delaylimit -= steps;
				if (delaylimit == 0)
					break;

				// pulse 2
				steps = std::min<int>(pulse2, delaylimit);
				nextticks += pulsedelaybase + pulsedelayscale * steps;
				OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
				prevticks = nextticks;

				delaylimit -= steps;
				if (delaylimit == 0)
					break;
			}
			while (true);

			// inner delay
			nextticks += innerdelaypost;
			OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
			prevticks = nextticks;

			// update pulse delays
			pulse1 = (pulse1 + pulse1innerdelta) & 0xFF;
			pulse2 = (pulse2 + pulse2innerdelta) & 0xFF;
		}


		// outer delay;
		nextticks += outerdelaypost;
		OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
		prevticks = nextticks;

		// update pulse delays
		pulse1delay = (pulse1delay + pulse1outerdelta) & 0xFF;
		pulse2delay = (pulse2delay + pulse2outerdelta) & 0xFF;
	}

	return true;
}

static size_t ReadBinaryData(const char *data, unsigned char buffer[], size_t size)
{
	size_t count = 0;
	size_t len = strlen(data);
	bool high = false;
	for (size_t i = 0; i < len; ++i)
	{
		unsigned char value;
		if (data[i] >= '0' && data[i] <= '9')
			value = unsigned char(data[i] - '0');
		else if (data[i] >= 'A' && data[i] <= 'F')
			value = unsigned char(data[i] - 'A' + 10);
		else if (data[i] >= 'a' && data[i] <= 'f')
			value = unsigned char(data[i] - 'a' + 10);
		else
		{
			high = false; continue;
		}

		if (high)
		{
			buffer[count] = (buffer[count] << 4) + value;
			++count;
			high = false;
		}
		else
		{
			buffer[count] = value;
			high = true;
		}
		if (count >= size)
			break;
	}
	return count;
}

static bool ConfigureWaveLoop(SoundTemplate *self, const TiXmlElement *element, unsigned int id)
{
	// clock frequency
	int frequency = 3579000;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 4;
	element->QueryFloatAttribute("divider", &divider);

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;


	/*
	;;; parameter table
	A=00 (sound 0x01):	FEEA	81 24 00 00 00 16 31	; enemy kill
	A=01 (sound 0x02):	FEF1	12 05 1A FF 00 27 6D	; space guppy fire
	A=02 (sound 0x03):	FEF8	11 05 11 01 0F 01 47	; lander fire
	A=03 (sound 0x04):	FEFF	11 31 00 01 00 0D 1B	; ???
	A=04 (sound 0x05):	FF06	F4 12 00 00 00 14 47	; pod explosion
	A=05 (sound 0x06):	FF0D	41 45 00 00 00 0F 5B	; lander kill
	A=06 (sound 0x07):	FF14	21 35 11 FF 00 0D 1B	; space guppy kill
	A=07 (sound 0x08):	FF1B	15 00 00 FD 00 01 69	; humanoid catch
	A=08 (sound 0x09):	FF22	31 11 00 01 00 03 6A	; firebomber/mutant fire
	A=09 (sound 0x0A):	FF29	01 15 01 01 01 01 47	; game start
	A=0A (sound 0x0B):	FF30	F6 53 03 00 02 06 94	; humanoid abduction
	A=0B (sound 0x0C):	FF37	6A 10 02 00 02 06 9A	; swarmer fire
	A=0C (sound 0x0D):	FF3E	1F 12 00 FF 10 04 69	; humanoid deposit, also used by sound 0x12


	; packed wave data table
	; variable-sized records: size | data[0] .. data[size-1]
	I=0:	FE4B	08 | 7F D9 FF D9 7F 24 00 24
	I=1:	FE54	08 | 00 40 80 00 FF 00 80 40
	I=2:	FE5D	10 | 7F B0 D9 F5 FF F5 D9 B0 7F 4E 24 09 00 09 24 4E
	I=3:	FE6E	10 | 7F C5 EC E7 BF 8D 6D 6A 7F 94 92 71 40 17 12 39
	I=4:	FE7F	10 | FF FF FF FF 00 00 00 00 FF FF FF FF 00 00 00 00
	I=5:	FE90	48 | 8A 95 A0 AB B5 BF C8 D1 DA E1 E8 EE F3 F7 FB FD
						 FE FF FE FD FB F7 F3 EE E8 E1 DA D1 C8 BF B5 AB
						 A0 95 8A 7F 75 6A 5F 54 4A 40 37 2E 25 1E 17 11
						 0C 08 04 02 01 00 01 02 04 08 0C 11 17 1E 25 2E
						 37 40 4A 54 5F 6A 75 7F
	I=6:	FED9	10 | 59 7B 98 AC B3 AC 98 7B 59 37 19 06 00 06 19 37


	; packed period data table
	+1B:	FF6E	0D | 01 01 02 02 03 04 05 06 07 08 09 0A
	+31:	FF84	16 | 01 01 02 02 04 04 08 08 10 20 28 30 38 40 48 50
						 60 70 80 A0 B0 C0
	+47:	FF9A	14 | 08 40 08 40 08 40 08 40 08 40 08 40 08 40 08 40
						 08 40 08 40
	+5B:	FFAE	0F | 01 02 04 08 09 0A 0B 0C 0E 0F 10 12 14 16 40
	+69:	FFBC	04 | 40 10 08 01
	+6A:	FFBD	03 | 10 08 01
	+6D:	FFC0	27 | 01 01 01 01 02 02 03 03 04 04 05 06 08 0A 0C 10
						 14 18 20 30 40 50 40 30 20 10 0C 0A 08 07 06 05
						 04 03 02 02 01 01 01
	+94:	FFE7	06 | 07 08 09 0A 0C 08
	+9A:	FFED	06 | 17 18 19 1A 1B 1C


	;;; input values
	[0x07]: set by IRQ handler to [0x04]|[0x05]

	;;; unpacked parameters
	[0x13]: high nybble of record[0]		(period table repeat count)
	[0x14]: low nybble of record[0]			(wave table repeat count)
	[0x15]: high nybble of record[1]		(fade delta per period table repeat)
	[0x16]: record[3]						(period offset delta)
	[0x17]: record[4]						(outer loop count)
	[0x18,0x19]: address of wave data entry indexed by low nybble of record[1]
	[0x1A]: record[2]						(fade start)
	[0x1B,0x1C]: 0xFF53 + record[6]			(start of period data)
	[0x1D,0x1E]: 0xFF53 + record[6] + record[5]	(end of period data)
	[0x24...]: copied wave data

	;;; local values
	[0x0D,0x0E] stored period data pointer
	[0x0F,0x10] stored wave data pointer
	[0x1F,0x20] end of copied wave data
	[0x21]: current period value
	[0x22]: current period repeat counter
	[0x23]: period add
	*/

	unsigned char B;
	unsigned char output = 0;

#if 1
	unsigned int wavelength = 0;
	unsigned char wavesource[0x80] = { 0 };

	unsigned int pitchlength = 0;
	unsigned char pitchsource[0x40] = { 0 };

	int pitchrepeat = 0;		// [0x13], high nybble of record[0]
	element->QueryIntAttribute("delayrepeat", &pitchrepeat);

	int waverepeat = 0;			// [0x14], low nybble of record[0]
	element->QueryIntAttribute("waverepeat", &waverepeat);
		
	int fadedelta = 0;			// [0x15], high nybble of record[1]
	element->QueryIntAttribute("fadedelta", &fadedelta);

	int fadestart = 0;			// [0x1A], record[2]
	element->QueryIntAttribute("fadestart", &fadestart);

	int pitchdelta = 0;			// [0x16], record[3]
	element->QueryIntAttribute("delaydelta", &pitchdelta);

	int outerrepeat = 0;		// [0x17], record[4]
	element->QueryIntAttribute("outerrepeat", &outerrepeat);

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch(Hash(child->Value()))
		{
		case 0xa9f017d4 /* "wave" */:
			{
				if (const char *data = child->Attribute("data"))
					wavelength = ReadBinaryData(data, wavesource, SDL_arraysize(wavesource));

				for (const TiXmlElement *data = child->FirstChildElement(); data != NULL; data = data->NextSiblingElement())
				{
					switch(Hash(data->Value()))
					{
					case 0xd872e2a5 /* "data" */:
						{
							int value = 0;
							data->QueryIntAttribute("value", &value);
							wavesource[wavelength++] = unsigned char(value);
						}
						break;
					}
				}
			}
			break;

		case 0x4ed1f1d8 /* "delay" */:
			{
				if (const char *data = child->Attribute("data"))
					pitchlength = ReadBinaryData(data, pitchsource, SDL_arraysize(pitchsource));

				for (const TiXmlElement *data = child->FirstChildElement(); data != NULL; data = data->NextSiblingElement())
				{
					switch(Hash(data->Value()))
					{
					case 0xd872e2a5 /* "data" */:
						{
							int value = 0;
							data->QueryIntAttribute("value", &value);
							pitchsource[pitchlength++] = unsigned char(value);
						}
						break;
					}
				}
			}
			break;
		}
	}

#endif

	unsigned char *pitchbufferstart = pitchsource, *pitchbufferend = pitchsource + pitchlength;

	// clear delay offset
	int pitchoffset = 0;

	// sound generator loop
	do
	{
		int fade = fadestart;

		// for each iteration of the pitch loop
		for (int pitchloop = pitchrepeat; pitchloop != 0; --pitchloop)
		{
			nextticks += 3+5;

			// for each entry in the delay table...
			for (unsigned char *pitchptr = pitchbufferstart; pitchptr != pitchbufferend; ++pitchptr)
			{
				// compute pitch value
				int pitch = *pitchptr + pitchoffset;

				nextticks += 4+5+3+4+4+4+3+4+5;

				// set repeat counter
				for (int waveloop = waverepeat; waveloop != 0; --waveloop)
				{
					// output one wave pattern
					nextticks += 3;
					for (unsigned char *waveptr = wavesource; waveptr != wavesource + wavelength; ++waveptr)
					{
						nextticks += 3+(2+4)*pitch+5;

						samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
						prevticks = nextticks;

						output = unsigned char(*waveptr - fade * (*waveptr >> 4));

						nextticks += 5+4+4+4;
					}

					nextticks += 2+4;
					nextticks += 6*4+2*2+4;
				}

				nextticks -= 6*4+2*2+4;	// cancel delay from last iteration

				// TO DO: compute cycle count
				samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
				prevticks = nextticks;
			}

			// fade copied wave by fadedelta
			fade += fadedelta;
			nextticks += 3+5+6+4;
			nextticks += fadedelta ? 27+wavelength*(65+fadedelta*12) : 11;
		}

		// get delay offset delta
		if (pitchdelta == 0)
			break;

		// decrement repeat counter
		--outerrepeat;
		if (outerrepeat == 0)
			break;

		// TO DO: compute cycle count

		// apply delta to delay offset value
		pitchoffset += pitchdelta;

		// adjust delay table
		unsigned char *pitchptr = pitchbufferstart;

		B = 0;
		nextticks += 4+4+4+4+4+4+3+4+4+2;
		nextticks -= (3+6+4+5+4+4+2+4+4+4)*(pitchptr-pitchsource);
		do
		{
			// get pitch table entry plus pitch offset
			unsigned int pitch = unsigned char(pitchoffset) + *pitchptr;

			// if the pitch offset delta is positive...
			if (pitchdelta >= 0)
			{
				// if the combined pitch wraps around...
				if (pitch > 0xFF)
					goto FC27;
				goto FC2C;
			}

			// if the combined pitch reaches zero...
			if ((pitch & 0xFF) == 0)
				goto FC27;
			if (pitch > 0xFF)
				goto FC2C;
FC27:
			if (B == 0)
				continue;

			// set end of table
			pitchbufferend = pitchptr;
			break;
FC2C:
			if (B != 0)
				continue;

			// set beginning of table
			pitchbufferstart = pitchptr;
			++B;
		}
		while (++pitchptr < pitchbufferend);
		nextticks += (3+6+4+5+4+4+2+4+4+4)*(pitchptr-pitchsource);

		if (B == 0)
			break;

		nextticks += 2+4;

		// if applying fade...
		if (fadedelta != 0)
		{
			// reinitialize wave data
			nextticks += 5+57+43*wavelength;
			nextticks += 3;
			nextticks += fadestart ? 5+27+wavelength*(65+fadestart*12) : 5+11;
		}
		nextticks += 5;
	}
	while (1);

	samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
	prevticks = nextticks;
	return true;
}

// humanoid falling
static bool ConfigureStargateF9F3(SoundTemplate *self, const TiXmlElement *element, unsigned int id)
{
	// clock frequency
	int frequency = 3579000;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 4;
	element->QueryFloatAttribute("divider", &divider);

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;


// jump table entry 0x0C (sound 0x1A)
	unsigned char A, B;
	unsigned short X;
	unsigned char output = 0;
	unsigned char mem_0x11 = 0;
	unsigned char mem_0x12 = 0;
	unsigned char mem_0x13[10] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned short length = 8;

	do
	{
		do
		{
			X = 0;
			mem_0x11 = 0x80;
			B = 0;
			nextticks += 3+2+4+2;
			do
			{
				mem_0x13[X+1] += mem_0x13[X+0];
				if (char(mem_0x13[X+1]) < 0)
					B += mem_0x11;
				mem_0x11 >>= 1;
				++X;
				++X;
			}
			while (X < length);

			nextticks += 4*(5+5+6+4+6+4+4+3+4);
			samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
			prevticks = nextticks;

			output = B;

			nextticks += 5+6+4;
		}
		while (++mem_0x12 != 0);

		X = 0;
		B = 0;
		nextticks += 5;
		do
		{
			A = mem_0x13[X+0];
			nextticks += 5+4;
			if (A != 0)
			{
				nextticks += 2+4;
				if (A == 0x37)
				{
					nextticks += 2+6;
					B = 0x41;
					mem_0x13[X+2] = B;
				}
				nextticks += 7+2;
				--mem_0x13[X+0];
				++B;
			}
			nextticks += 4+4+3+4;
			++X;
			++X;
		}
		while (X < length);

		nextticks += 2+4;
	}
	while (B != 0);

	samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
	prevticks = nextticks;

	return true;
}

static bool ConfigureExpression(SoundTemplate *self, const TiXmlElement *element, unsigned int id)
{
	// get sound length
	float length;
	element->QueryFloatAttribute("length", &length);

	// allocate space
	size_t count = xs_CeilToInt(length * AUDIO_FREQUENCY);
	self->mSize = (self->mLength + count) * sizeof(short);
	self->mData = realloc(self->mData, self->mSize);

	// get expression
	std::vector<unsigned int> buffer;
	ConfigureExpressionRoot<float>(element, buffer, sScalarNames, sScalarDefault);

	// set up a context
	EntityContext context(&buffer[0], buffer.size(), 0, id);

	// for each sample...
	for (size_t i = 0; i < count; ++i, context.Restart())
	{
		// evaluate the expression
		context.mParam = float(i) / AUDIO_FREQUENCY;
		float value = Expression::Evaluate<float>(context);

		// add a sample
		static_cast<short *>(self->mData)[self->mLength++] = short(Clamp(xs_RoundToInt(value * SHRT_MAX), SHRT_MIN, SHRT_MAX));
	}

	return true;
}


bool SoundTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	// process sound configuration
	for (const TiXmlElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x0e0d9594 /* "sound" */:
			{
				// get sound template
				const char *name = child->Attribute("name");
				const SoundTemplate &sound = Database::soundtemplate.Get(Hash(name));
				if (sound.mLength)
				{
					// append sound data
					mSize = (mLength + sound.mLength) * sizeof(short);
					mData = realloc(mData, mSize);
					memcpy(static_cast<short *>(mData) + mLength, sound.mData, sound.mLength * sizeof(short));
				}
			}
			break;

		case 0xaaea5743 /* "file" */:
			ConfigureFile(child, id);
			break;

		case 0x96e382a7 /* "sample" */:
			ConfigureSample(child, id);
			break;

		case 0xe8f2b85f /* "pokey" */:
			ConfigurePokey(child, id);
			break;

		case 0xcc6424ae /* "bitnoiseramp" */:
			ConfigureBitNoiseRamp(this, child, id);
			break;

		case 0x0a2a7b91 /* "trianglenoise" */:
			ConfigureTriangleNoise(this, child, id);
			break;

		case 0x920eedb8 /* "StargateF9A6" */:
			ConfigureStargateF9A6(this, child, id);
			break;

		case 0x36f16844 /* "pulseloop" */:
			ConfigurePulseLoop(this, child, id);
			break;

		case 0xde15262a /* "waveloop" */:
			ConfigureWaveLoop(this, child, id);
			break;

		case 0x071661ac /* "StargateF9F3" */:
			ConfigureStargateF9F3(this, child, id);
			break;

		case 0xcf15afeb /* "expression" */:
			ConfigureExpression(this, child, id);
			break;
		}
	}

	// output total length
	DebugPrint("size=%d length=%d (%fs)\n", mSize, mLength, float(mLength) / AUDIO_FREQUENCY);

#ifdef _DEBUG
//#define DEBUG_OUTPUT_SOUND_FILE
#endif
#ifdef DEBUG_OUTPUT_SOUND_FILE
	// debug
	FILE *file = fopen("test.wav", "wb");

	struct RIFF_header
	{
		unsigned int ChunkID;
		unsigned int ChunkSize;
		unsigned int Format;
	}
	riff_header =
	{
		0x46464952,
		36 + mLength * sizeof(short),
		0x45564157
	};
	fwrite(&riff_header, sizeof(riff_header), 1, file);

	struct FMT_header
	{
		unsigned int Subchunk1ID;
		unsigned int Subchunk1Size;
		unsigned short AudioFormat;
		unsigned short NumChannels;
		unsigned int SampleRate;
		unsigned int ByteRate;
		unsigned short BlockAlign;
		unsigned short BitsPerSample;
	}
	fmt_header =
	{
		0x20746d66,
		16,
		1,
		1,
		48000,
		48000 * sizeof(short),
		sizeof(short),
		16
	};
	fwrite(&fmt_header, sizeof(fmt_header), 1, file);

	struct DATA_header
	{
		unsigned int Subchunk2ID;
		unsigned int Subchunk2Size;
	}
	data_header =
	{
		0x61746164,
		mLength * sizeof(short)
	};
	fwrite(&data_header, sizeof(data_header), 1, file);

	fwrite(mData, sizeof(short), mLength, file);

	fclose(file);
#endif

	return true;
}

static Sound *sHead;
static Sound *sTail;
static Sound *sNext;

Sound::Sound(void)
: Updatable(0)
, mNext(NULL)
, mPrev(NULL)
, mData(NULL)
, mLength(0)
, mOffset(0)
, mVolume(0)
, mRepeat(0)
, mPosition(0, 0)
#if defined(USE_SDL_MIXER)
, mPlaying(-1)
#else
, mPlaying(false)
#endif
{
	SetAction(Action(this, &Sound::Update));
}

Sound::Sound(const SoundTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mNext(NULL)
, mPrev(NULL)
#if defined(USE_SDL_MIXER)
, mData(Mix_QuickLoad_RAW(static_cast<unsigned char *>(aTemplate.mData), aTemplate.mSize))
#else
, mData(aTemplate.mData)
#endif
, mLength(aTemplate.mLength)
, mOffset(0)
, mVolume(aTemplate.mVolume)
, mRepeat(aTemplate.mRepeat)
, mPosition(0, 0)
#if defined(USE_SDL_MIXER)
, mPlaying(-1)
#else
, mPlaying(false)
#endif
{
	SetAction(Action(this, &Sound::Update));
}

Sound::~Sound(void)
{
	Stop();
}


void Sound::Play(unsigned int aOffset)
{
#if defined(USE_SDL_MIXER)
	if (mLength <= 0)
		return;
	if (mPlaying >= 0)
		Mix_HaltChannel(mPlaying);
	mPlaying = Mix_PlayChannel(-1, static_cast<Mix_Chunk *>(mData), mRepeat);
	if (mPlaying >= 0)
		Update(0.0f);
	else
		DebugPrint("%s\n", Mix_GetError());
#elif defined(USE_SDL)
	if (!mPlaying)
	{
		SDL_LockAudio();
		mPlaying = true;
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
		SDL_UnlockAudio();
	}
#endif

	mOffset = aOffset;

	if (mPlaying >= 0)
	{
		// also activate
		Activate();
	}
}

void Sound::Stop(void)
{
#if defined(USE_SDL_MIXER)
	if (mPlaying >= 0)
	{
		Mix_HaltChannel(mPlaying);
		mPlaying = -1;
	}
#elif defined(USE_SDL)
	if (mPlaying)
	{
		SDL_LockAudio();
		mPlaying = false;
		if (sHead == this)
			sHead = mNext;
		if (sTail == this)
			sTail = mPrev;
		if (sNext == this)
			sNext = mNext;
		if (mNext)
			mNext->mPrev = mPrev;
		if (mPrev)
			mPrev->mNext = mNext;
		mNext = NULL;
		mPrev = NULL;
		SDL_UnlockAudio();
	}
#endif

	// also deactivate
	Deactivate();
}

// hack!
void Sound::Update(float aStep)
{
#if defined(USE_SDL_MIXER)
	if (!Mix_Playing(mPlaying))
	{
		mPlaying = -1;
		Deactivate();
		return;
	}
#else
	if (!mRepeat && mOffset >= mLength)
	{
		Stop();
		return;
	}
#endif

#if defined(DISTANCE_FALLOFF)
	if (Entity *entity = Database::entity.Get(mId))
		mPosition = entity->GetPosition();
	else
		mPosition = listenerpos;
#endif

#if defined(USE_SDL_MIXER)
	if (mPlaying >= 0)
	{
		float volume = mVolume * SOUND_VOLUME_EFFECT;
#if defined(DISTANCE_FALLOFF)
		if (mId)
		{
			// apply sound fall-off
			volume = DISTANCE_FALLOFF(volume, listenerpos.DistSq(mPosition));
		}
#endif
		Mix_Volume(mPlaying, xs_RoundToInt(volume * MIX_MAX_VOLUME));
	}
#endif
}


// AUDIO SYSTEM

// initialize
void Sound::Init(void)
{
#if defined(USE_SDL_MIXER)
	if ( Mix_OpenAudio(AUDIO_FREQUENCY, AUDIO_S16SYS, 1, xs_CeilToInt(AUDIO_FREQUENCY / SIMULATION_RATE) ) < 0 )
	{
		DebugPrint("Unable to open audio: %s\n", Mix_GetError());
		return;
	}
	Mix_AllocateChannels(256);
	UpdateSoundVolume();
#elif defined(USE_SDL)
	SDL_AudioSpec fmt;
	fmt.freq = AUDIO_FREQUENCY;
	fmt.format = AUDIO_S16SYS;
	fmt.channels = 2;
	fmt.samples = Uint16(AUDIO_FREQUENCY / SIMULATION_RATE);
	fmt.callback = MixSound;
	fmt.userdata = &Sound::listenerpos;

	/* Open the audio device and start playing sound! */
	if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
		DebugPrint("Unable to open audio: %s\n", SDL_GetError());
	}
#endif
}

// clean up
void Sound::Done(void)
{
#if defined(USE_SDL_MIXER)
	Mix_CloseAudio();
#elif defined(USE_SDL)
	SDL_CloseAudio();
#endif
}

// pause
void Sound::Pause(void)
{
#if defined(USE_SDL_MIXER)
	Mix_Pause(-1);
	Mix_PauseMusic();
#elif defined(USE_SDL)
	SDL_PauseAudio(true);
#endif
}

// resume
void Sound::Resume(void)
{
#if defined(USE_SDL_MIXER)
	Mix_Resume(-1);
	Mix_ResumeMusic();
#elif defined(USE_SDL)
	SDL_PauseAudio(false);
#endif
}



#if defined(USE_SDL) && !defined(USE_SDL_MIXER)

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
		// if associated with an identifier
		if (sound->mId)
		{
			// apply sound fall-off
			volume = DISTANCE_FALLOFF(volume, listenerpos.DistSq(sound->mPosition));
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

void UpdateSoundVolume(void)
{
#ifdef USE_SDL_MIXER
	Mix_VolumeMusic(xs_CeilToInt(SOUND_VOLUME_MUSIC * MIX_MAX_VOLUME));
#endif
}

void PlaySoundCue(unsigned int aId, unsigned int aCueId)
{
	const Database::Typed<unsigned int> &soundcues = Database::soundcue.Get(aId);
	unsigned int aSoundId = soundcues.Get(aCueId);
	const SoundTemplate &soundtemplate = Database::soundtemplate.Get(aSoundId);
	if (soundtemplate.mSize > 0)
	{
		/* Put the sound data in the slot (it starts playing immediately) */
		Database::Typed<Sound *> &sounds = Database::sound.Open(aId);
		if (Sound *s = sounds.Get(aCueId))
		{
			// retrigger
			s->Play(0);
		}
		else
		{
			// start new
			s = new Sound(soundtemplate, aId);
			sounds.Put(aCueId, s);
			s->Play(0);
		}
	}
}

void StopSoundCue(unsigned int aId, unsigned int aCueId)
{
	const Database::Typed<Sound *> &sounds = Database::sound.Get(aId);
	if (Sound *s = sounds.Get(aCueId))
	{
		// stop
		s->Stop();
	}
}
