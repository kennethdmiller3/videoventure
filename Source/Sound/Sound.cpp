#include "StdAfx.h"
#include "Sound.h"
#include "SoundConfigure.h"
#include "Entity.h"

#define DISTANCE_FALLOFF

#if defined(USE_BASS)

#include "bass.h"

#elif defined(USE_SDL_MIXER)

#include "SDL_Mixer.h"

#else

#include "SoundMixer.h"

#endif


// sound attributes
int SOUND_CHANNELS = 8;				// effect mixer channels
float SOUND_VOLUME_EFFECT = 0.5f;	// effects volume
float SOUND_VOLUME_MUSIC = 0.5f;	// music volume

// sound position factors
float SOUND_DISTANCE_FACTOR = 1.0f/16.0f;
float SOUND_ROLLOFF_FACTOR = 0.0f;
float SOUND_DOPPLER_FACTOR = 0.0f;
extern float CAMERA_DISTANCE;

// sound listener position
Vector2 Sound::listenerpos;
Vector2 Sound::listenervel;


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

#if defined(USE_BASS)
static const char * BASS_ErrorGetString()
{
	const char * const sCodeString[] =
	{
		"ERROR_UNKNOWN",	// BASS_ERROR_UNKNOWN	-1	// some other mystery problem
		"OK",				// BASS_OK				0	// all is OK
		"ERROR_MEM",		// BASS_ERROR_MEM		1	// memory error
		"ERROR_FILEOPEN",	// BASS_ERROR_FILEOPEN	2	// can't open the file
		"ERROR_DRIVER",		// BASS_ERROR_DRIVER	3	// can't find a free/valid driver
		"ERROR_BUFLOST",	// BASS_ERROR_BUFLOST	4	// the sample buffer was lost
		"ERROR_HANDLE",		// BASS_ERROR_HANDLE	5	// invalid handle
		"ERROR_FORMAT",		// BASS_ERROR_FORMAT	6	// unsupported sample format
		"ERROR_POSITION",	// BASS_ERROR_POSITION	7	// invalid position
		"ERROR_INIT",		// BASS_ERROR_INIT		8	// BASS_Init has not been successfully called
		"ERROR_START",		// BASS_ERROR_START		9	// BASS_Start has not been successfully called
		"",
		"",
		"",
		"",
		"ERROR_ALREADY",	// BASS_ERROR_ALREADY	14	// already initialized/paused/whatever
		"",
		"",
		"",
		"ERROR_NOCHAN",		// BASS_ERROR_NOCHAN	18	// can't get a free channel
		"ERROR_ILLTYPE",	// BASS_ERROR_ILLTYPE	19	// an illegal type was specified
		"ERROR_ILLPARAM",	// BASS_ERROR_ILLPARAM	20	// an illegal parameter was specified
		"ERROR_NO3D",		// BASS_ERROR_NO3D		21	// no 3D support
		"ERROR_NOEAX",		// BASS_ERROR_NOEAX		22	// no EAX support
		"ERROR_DEVICE",		// BASS_ERROR_DEVICE	23	// illegal device number
		"ERROR_NOPLAY",		// BASS_ERROR_NOPLAY	24	// not playing
		"ERROR_FREQ",		// BASS_ERROR_FREQ		25	// illegal sample rate
		"",
		"ERROR_NOTFILE",	// BASS_ERROR_NOTFILE	27	// the stream is not a file stream
		"",
		"ERROR_NOHW",		// BASS_ERROR_NOHW		29	// no hardware voices available
		"",
		"ERROR_EMPTY",		// BASS_ERROR_EMPTY		31	// the MOD music has no sequence data
		"ERROR_NONET",		// BASS_ERROR_NONET		32	// no internet connection could be opened
		"ERROR_CREATE",		// BASS_ERROR_CREATE	33	// couldn't create the file
		"ERROR_NOFX",		// BASS_ERROR_NOFX		34	// effects are not available
		"",
		"",
		"ERROR_NOTAVAIL",	// BASS_ERROR_NOTAVAIL	37	// requested data is not available
		"ERROR_DECODE",		// BASS_ERROR_DECODE	38	// the channel is a "decoding channel"
		"ERROR_DX",			// BASS_ERROR_DX		39	// a sufficient DirectX version is not installed
		"ERROR_TIMEOUT",	// BASS_ERROR_TIMEOUT	40	// connection timedout
		"ERROR_FILEFORM",	// BASS_ERROR_FILEFORM	41	// unsupported file format
		"ERROR_SPEAKER",	// BASS_ERROR_SPEAKER	42	// unavailable speaker
		"ERROR_VERSION",	// BASS_ERROR_VERSION	43	// invalid BASS version (used by add-ons)
		"ERROR_CODEC",		// BASS_ERROR_CODEC		44	// codec is not available/supported
		"ERROR_ENDED",		// BASS_ERROR_ENDED		45	// the channel/file has ended
	};
	int code = BASS_ErrorGetCode();
	return sCodeString[code+1];
}
#endif

namespace Database
{
	Typed<SoundTemplate> soundtemplate(0x1b5ef1be /* "soundtemplate" */);
	Typed<Typed<unsigned int> > soundcue(0xf23cbd5f /* "soundcue" */);
	Typed<Typed<Sound *> > sound(0x0e0d9594 /* "sound" */);
	Typed<std::string> musictemplate(0x19706bfe /* "musictemplate" */);
#if defined(USE_BASS)
	Typed<HMUSIC> music(0x9f9c4fd4 /* "music" */);
#elif defined(USE_SDL_MIXER)
	Typed<Mix_Music *> music(0x9f9c4fd4 /* "music" */);
#endif

	namespace Loader
	{
		class SoundSystemLoader
		{
		public:
			SoundSystemLoader()
			{
				AddConfigure(0x01ba332d /* "soundsystem" */, Entry(this, &SoundSystemLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0x2eb31462 /* "distance" */:
						child->QueryFloatAttribute("factor", &SOUND_DISTANCE_FACTOR);
						break;

					case 0xab59d4bb /* "rolloff" */:
						child->QueryFloatAttribute("factor", &SOUND_ROLLOFF_FACTOR);
						break;

					case 0xcca0ad5f /* "doppler" */:
						child->QueryFloatAttribute("factor", &SOUND_DOPPLER_FACTOR);
						break;
					}
				}

#if defined(USE_BASS)
				BASS_Set3DFactors(SOUND_DISTANCE_FACTOR, SOUND_ROLLOFF_FACTOR, SOUND_DOPPLER_FACTOR);
				BASS_Apply3D();
#endif
			}
		}
		soundsystemloader;

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

				// if there are no sound cues...
				if (!Database::soundcue.Find(aId))
				{
					// add a default cue (HACK)
					Typed<unsigned int> &soundcue = Database::soundcue.Open(aId);
					soundcue.Put(0, aId);
					Database::soundcue.Close(aId);
				}

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
				if (Database::sound.Find(aId))
				{
#if defined(USE_SDL)
					SDL_LockAudio();
#endif
					const Typed<Sound *> &sounds = Database::sound.Get(aId);
					for (Typed<Sound *>::Iterator itor(&sounds); itor.IsValid(); ++itor)
						delete itor.GetValue();
					Database::sound.Delete(aId);
#if defined(USE_SDL)
					SDL_UnlockAudio();
#endif
				}
			}
		}
		soundinitializer;

#if defined(USE_BASS)
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
				HMUSIC handle = BASS_MusicLoad(false, music.c_str(), 0, 0, BASS_MUSIC_RAMPS|BASS_MUSIC_LOOP, 0);
				if (!handle)
					DebugPrint("error loading music: %s\n", BASS_ErrorGetString());
				if (!BASS_ChannelPlay(handle, true))
					DebugPrint("error starting music: %s\n", BASS_ErrorGetString());
				Database::music.Put(aId, handle);
			}

			void Deactivate(unsigned int aId)
			{
				if (HMUSIC handle = Database::music.Get(aId))
				{
					BASS_ChannelStop(handle);
					BASS_MusicFree(handle);
					Database::music.Delete(aId);
				}
			}
		}
		musicinitializer;
#elif defined(USE_SDL_MIXER)
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
, mNear(CAMERA_DISTANCE)
, mFar(FLT_MAX)
, mRepeat(0)
#if defined(USE_BASS)
, mFrequency(AUDIO_FREQUENCY)
, mHandle(0)
#elif defined(USE_SDL_MIXER)
, mChunk(NULL)
#endif
{
}

SoundTemplate::SoundTemplate(const SoundTemplate &aTemplate)
: mData(malloc(aTemplate.mSize))
, mSize(aTemplate.mSize)
, mLength(aTemplate.mLength)
, mVolume(aTemplate.mVolume)
, mNear(aTemplate.mNear)
, mFar(aTemplate.mFar)
, mRepeat(aTemplate.mRepeat)
#if defined(USE_BASS)
, mFrequency(aTemplate.mFrequency)
, mHandle(0)
#elif defined(USE_SDL_MIXER)
, mChunk(aTemplate.mChunk ? Mix_QuickLoad_RAW(aTemplate.mChunk->abuf, aTemplate.mChunk->alen) : NULL)
#endif
{
	memcpy(mData, aTemplate.mData, mSize);
}

SoundTemplate::~SoundTemplate(void)
{
#if defined(USE_BASS)
	if (mHandle)
		BASS_SampleFree(mHandle);
#elif defined(USE_SDL_MIXER)
	if (mChunk)
		Mix_FreeChunk(mChunk);
#endif
	free(mData);
}

static class SoundFile
{
public:
	SoundFile()
	{
		SoundConfigure::Add(0xaaea5743 /* "file" */, Configure);
	}
	static bool Configure(SoundTemplate &self, const TiXmlElement *element, unsigned int id)
	{
		const char *name = element->Attribute("name");
		if (name == NULL)
			return false;

#if defined(USE_BASS)

	// load sound file
	HSAMPLE handle = BASS_SampleLoad(false, name, 0, 0, 1, BASS_SAMPLE_MONO);
	if (!handle)
	{
		DebugPrint("error loading sound file \"%s\": %s\n", name, BASS_ErrorGetString());
		return false;
	}

	// get sample info
	BASS_SAMPLE info;
	BASS_SampleGetInfo(handle, &info);

	// allocate space for data
	self.mSize = self.mLength * sizeof(short) + info.length * ((info.flags & BASS_SAMPLE_8BITS) ? sizeof(short) : 1) / info.chans;
	self.mData = realloc(self.mData, self.mSize);

	// set frequency
	self.mFrequency = info.freq;

	// if converting format...
	if ((info.chans > 1) || (info.flags & BASS_SAMPLE_8BITS))
	{
		// get sample data
		void *buf = _alloca(info.length);
		BASS_SampleGetData(handle, buf);

		// if converting from 8-bit...
		int accum = 0;
		if (info.flags & BASS_SAMPLE_8BITS)
		{
			for (unsigned int in = 0, out = self.mLength, samp = 0; in < info.length; ++in)
			{
				accum += static_cast<unsigned char *>(buf)[in];
				if (++samp >= info.chans)
				{
					static_cast<short *>(self.mData)[out++] = short(accum * 257 / samp - 32768);
					accum = 0;
					samp = 0;
				}
			}
		}
		else
		{
			for (unsigned int in = 0, out = self.mLength, samp = 0; in < info.length / sizeof(short); ++in)
			{
				accum += static_cast<short *>(buf)[in];
				if (++samp >= info.chans)
				{
					static_cast<short *>(self.mData)[out++] = short(accum / samp);
					accum = 0;
					samp = 0;
				}
			}
		}
	}
	else
	{
		// copy sound data
		BASS_SampleGetData(handle, static_cast<short *>(self.mData) + self.mLength);
	}
	self.mLength = self.mSize / sizeof(short);

	// free loaded data
	BASS_SampleFree(handle);

#elif defined(USE_SDL_MIXER)

	// load sound file
	Mix_Chunk *loadchunk = Mix_LoadWAV(name);
	if (!loadchunk)
	{
		DebugPrint("error loading sound file \"%s\": %s\n", name, Mix_GetError());
		return false;
	}

	// copy sound data
	self.mSize = self.mLength * sizeof(short) + loadchunk->alen;
	self.mData = realloc(self.mData, self.mSize);
	memcpy(static_cast<short *>(self.mData) + self.mLength, loadchunk->abuf, loadchunk->alen);
	self.mLength = self.mSize / sizeof(short);

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
	self.mSize = self.mLength * sizeof(short) + dlen * cvt.len_mult;
	self.mData = realloc(self.mData, self.mSize);
	memcpy(static_cast<short *>(self.mData) + self.mLength, data, dlen);
	cvt.buf = reinterpret_cast<unsigned char *>(static_cast<short *>(mData) + mLength);
	cvt.len = dlen;

	// convert to final format
	SDL_ConvertAudio(&cvt);
	self.mLength += cvt.len_cvt / sizeof(short);
	self.mSize = self.mLength * sizeof(short);
	self.mData = realloc(self.mData, self.mSize);

	// release wave file data
	SDL_FreeWAV(data);

#endif

		return true;
	}
}
soundfileloader;

bool SoundTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	// clear sound data
	mData = NULL;
	mSize = 0;
	mLength = 0;

	// get sound properties
	element->QueryFloatAttribute("volume", &mVolume);
	element->QueryFloatAttribute("near", &mNear);
	element->QueryFloatAttribute("far", &mFar);
	element->QueryIntAttribute("repeat", &mRepeat);

	// process sound configuration
	for (const TiXmlElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
	{
		unsigned int hash = Hash(child->Value());
		const SoundConfigure::Entry &configure = SoundConfigure::Get(hash);
		if (configure)
			configure(*this, child, id);
	}

	// output total length
	DebugPrint("size=%d length=%d (%fs)\n", mSize, mLength, float(mLength) / AUDIO_FREQUENCY);

#if defined(USE_BASS)
	// create a sample
	unsigned int flags = BASS_SAMPLE_OVER_POS;
#if defined(DISTANCE_FALLOFF)
	flags |= BASS_SAMPLE_3D;
#endif
	if (mRepeat)
		flags |= BASS_SAMPLE_LOOP;

	mHandle = BASS_SampleCreate(mSize, mFrequency, 1, 3, flags);
	if (mHandle)
	{
		// set sample data
		BASS_SampleSetData(mHandle, mData);

		// set default volume
		BASS_SAMPLE info;
		BASS_SampleGetInfo(mHandle, &info);
		info.volume = mVolume;
		info.mindist = mNear;
		info.maxdist = mFar;
		BASS_SampleSetInfo(mHandle, &info);
	}
	else
	{
		DebugPrint("error creating sample: %s\n", BASS_ErrorGetString());
	}
#elif defined(USE_SDL_MIXER)
	// create a chunk
	mChunk = Mix_QuickLoad_RAW(static_cast<unsigned char *>(mData), mSize);
	if (mChunk)
	{
		// set default volume
		Mix_VolumeChunk(mChunk, xs_RoundToInt(mVolume * MIX_MAX_VOLUME));
	}
#endif

#ifdef _DEBUG
//#define DEBUG_OUTPUT_SOUND_FILE
#endif
#ifdef DEBUG_OUTPUT_SOUND_FILE
	// debug
	std::string name = "sound-";
	name += Database::name.Get(id);
	name += ".wav";
	FILE *file = fopen(name.c_str(), "wb");

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
#if defined(USE_BASS)
		mFrequency,
		mFrequency * sizeof(short),
#else
		48000,
		48000 * sizeof(short),
#endif
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
#if defined(USE_BASS)
, mHandle(0)
#elif defined(USE_SDL_MIXER)
, mChunk(NULL)
, mNear(CAMERA_DISTANCE)
, mFar(FLT_MAX)
#elif defined(USE_SDL)
, mData(NULL)
, mLength(0)
, mOffset(0)
#endif
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
#if defined(USE_BASS)
, mHandle(aTemplate.mHandle)
#elif defined(USE_SDL_MIXER)
, mChunk(aTemplate.mChunk)
, mNear(aTemplate.mNear)
, mFar(aTemplate.mFar)
#elif defined(USE_SDL)
, mData(aTemplate.mData)
, mLength(aTemplate.mLength)
, mOffset(0)
#endif
, mVolume(aTemplate.mVolume)
, mRepeat(aTemplate.mRepeat)
, mPosition(0, 0)
, mVelocity(0, 0)
#if defined(USE_BASS)
, mPlaying(0)
#elif defined(USE_SDL_MIXER)
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
#if defined(USE_BASS)
	if (mHandle == 0)
		return;
	if (mPlaying == 0)
		mPlaying = BASS_SampleGetChannel(mHandle, false);
	if (mPlaying == 0)
	{
		DebugPrint("error getting channel: %s\n", BASS_ErrorGetString());
		return;
	}
	if (!BASS_ChannelPlay(mPlaying, true))
	{
		DebugPrint("error playing sound: %s\n", BASS_ErrorGetString());
		return;
	}
	Update(0.0f);
	Activate();
#elif defined(USE_SDL_MIXER)
	if (!mChunk)
		return;
	if (mPlaying >= 0)
		Mix_HaltChannel(mPlaying);
	mPlaying = Mix_PlayChannel(-1, mChunk, mRepeat);
	if (mPlaying < 0)
	{
		DebugPrint("%s\n", Mix_GetError());
		return;
	}
	Update(0.0f);
	Activate();
#elif defined(USE_SDL)
	if (!mPlaying)
	{
		SDL_LockAudio();
		Activate();
		SDL_UnlockAudio();
	}
	mOffset = aOffset;
#endif
}

void Sound::Stop(void)
{
#if defined(USE_BASS)
	if (mPlaying != 0)
	{
		BASS_ChannelStop(mPlaying);
		mPlaying = 0;
	}
#elif defined(USE_SDL_MIXER)
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
#if defined(USE_BASS)
	if (BASS_ChannelIsActive(mPlaying) == BASS_ACTIVE_STOPPED)
	{
		Stop();
		return;
	}
#elif defined(USE_SDL_MIXER)
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
	{
		mPosition = entity->GetPosition();
		mVelocity = entity->GetVelocity();
	}
	else
	{
		mPosition = listenerpos;
		mVelocity = listenervel;
	}
#endif

#if defined(USE_BASS)
	if (mPlaying)
	{
//		BASS_ChannelSetAttribute(mPlaying, BASS_ATTRIB_VOL, mVolume);

#if defined(DISTANCE_FALLOFF)
		BASS_3DVECTOR pos(mPosition.x, mPosition.y, 0.0f);
		BASS_3DVECTOR vel(mVelocity.x, mVelocity.y, 0.0f);
		if (!BASS_ChannelSet3DPosition(mPlaying, &pos, NULL, &vel))
			DebugPrint("error setting channel 3d position: %s\n", BASS_ErrorGetString());
#endif
	}
#elif defined(USE_SDL_MIXER)
	if (mPlaying >= 0)
	{
		float volume = SOUND_VOLUME_EFFECT;
#if defined(DISTANCE_FALLOFF)
		if (mId && SOUND_ROLLOFF_FACTOR)
		{
			// get distance
			const float dist = sqrtf(listenerpos.DistSq(mPosition) + CAMERA_DISTANCE * CAMERA_DISTANCE);

			// if outside the near distance...
			if (dist > mNear)
			{
				// apply sound fall-off
				volume *= mNear / (mNear + SOUND_ROLLOFF_FACTOR * (dist - mNear));
			}
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
#if defined(USE_BASS)
	// setup output - get latency
	DWORD flags = BASS_DEVICE_LATENCY;
#if defined(DISTANCE_FALLOFF)
	flags |= BASS_DEVICE_3D;
#endif
	if (!BASS_Init(-1,AUDIO_FREQUENCY,flags,0,NULL))
	{
		DebugPrint("Can't initialize device");
		return;
	}

	// get info
	BASS_INFO info;
	BASS_GetInfo(&info);
	DebugPrint("device latency: %dms\n", info.latency);
	DebugPrint("device minbuf: %dms\n", info.minbuf);
	DebugPrint("ds version: %d (effects %s)\n", info.dsver, info.dsver<8 ? "disabled" : "enabled");

	// initialize sound volume
	UpdateSoundVolume();

#if defined(DISTANCE_FALLOFF)
	// set default 3D factors
	if (!BASS_Set3DFactors(SOUND_DISTANCE_FACTOR, SOUND_ROLLOFF_FACTOR, SOUND_DOPPLER_FACTOR))
		DebugPrint("error setting 3d factors: %s\n", BASS_ErrorGetString());
	BASS_Apply3D();
#endif
#elif defined(USE_SDL_MIXER)
	// initialize mixer
	if ( Mix_OpenAudio(AUDIO_FREQUENCY, AUDIO_S16SYS, 1, xs_CeilToInt(AUDIO_FREQUENCY / SIMULATION_RATE) ) < 0 )
	{
		DebugPrint("Unable to open audio: %s\n", Mix_GetError());
		return;
	}
	Mix_AllocateChannels(SOUND_CHANNELS);

	// initialize sound volume
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
#if defined(USE_BASS)
	BASS_Free();
#elif defined(USE_SDL_MIXER)
	Mix_CloseAudio();
#elif defined(USE_SDL)
	SDL_CloseAudio();
#endif
}

// pause
void Sound::Pause(void)
{
#if defined(USE_BASS)
	BASS_Pause();
#elif defined(USE_SDL_MIXER)
	Mix_Pause(-1);
	Mix_PauseMusic();
#elif defined(USE_SDL)
	SDL_PauseAudio(true);
#endif
}

// resume
void Sound::Resume(void)
{
#if defined(USE_BASS)
	BASS_Start();
#elif defined(USE_SDL_MIXER)
	Mix_Resume(-1);
	Mix_ResumeMusic();
#elif defined(USE_SDL)
	SDL_PauseAudio(false);
#endif
}

// update listener
void Sound::Listener(Vector2 aPos, Vector2 aVel)
{
#if defined(DISTANCE_FALLOFF)
	listenerpos = aPos;
	listenervel = aVel;

#if defined(USE_BASS)
	BASS_3DVECTOR pos(listenerpos.x, listenerpos.y, -CAMERA_DISTANCE);
	BASS_3DVECTOR vel(listenervel.x, listenervel.y, 0.0f);
	BASS_3DVECTOR front(0.0f, 1.0f, 0.0f);
	BASS_3DVECTOR top(0.0f, 0.0f, 1.0f);
	if (!BASS_Set3DPosition(&pos, &vel, &front, &top))
		DebugPrint("error setting listener 3d position: %s\n", BASS_ErrorGetString());
	BASS_Apply3D();
#endif
#endif
}


void UpdateSoundVolume(void)
{
#if defined(USE_BASS)
	BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, xs_CeilToInt(SOUND_VOLUME_EFFECT * 10000));
	BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, xs_CeilToInt(SOUND_VOLUME_MUSIC * 10000));
#elif defined(USE_SDL_MIXER)
	Mix_VolumeMusic(xs_CeilToInt(SOUND_VOLUME_MUSIC * MIX_MAX_VOLUME));
#endif
}

void GAME_API PlaySoundCue(unsigned int aId, unsigned int aCueId)
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

void GAME_API StopSoundCue(unsigned int aId, unsigned int aCueId)
{
	const Database::Typed<Sound *> &sounds = Database::sound.Get(aId);
	if (Sound *s = sounds.Get(aCueId))
	{
		// stop
		s->Stop();
	}
}
