#include "StdAfx.h"
#include "Sound.h"
#include "SoundConfigure.h"
#include "Entity.h"

#define DISTANCE_FALLOFF
#define PROFILE_SOUND_SYNTHESIS

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
static MemoryPool sPool(sizeof(Sound));
void *Sound::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Sound::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif

#if defined(USE_BASS)
static const char * const sErrorCodeString[] =
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
	"ERROR_BUSY",		// BASS_ERROR_BUSY		46	// the device is busy
};
const char * BASS_ErrorGetString()
{
	int code = BASS_ErrorGetCode();
	return sErrorCodeString[code+1];
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

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
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

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
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
				for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0xe5561300 /* "cue" */:
						{
							// assign cue
							const char *name = child->Attribute("name");
							unsigned int subid = Hash(name);
							if (name)
								Database::name.Put(subid, name);
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

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
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

bool SoundTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int id)
{
#ifdef PROFILE_SOUND_SYNTHESIS
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
#endif

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
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
	{
		unsigned int hash = Hash(child->Value());
		const SoundConfigure::Entry &configure = SoundConfigure::Get(hash);
		if (configure)
		{
#ifdef PROFILE_SOUND_SYNTHESIS
			LARGE_INTEGER count0;
			QueryPerformanceCounter(&count0);
#endif
			configure(*this, child, id);
#ifdef PROFILE_SOUND_SYNTHESIS
			LARGE_INTEGER count1;
			QueryPerformanceCounter(&count1);

			DebugPrint("%s %dus\n", child->Value(), (count1.QuadPart - count0.QuadPart) * 1000000 / freq.QuadPart);
#endif
		}
	}

	// trim excess space
	Trim();

	// output total length
	DebugPrint("size=%d length=%d (%fs)\n", mSize, mLength, float(mLength) / AUDIO_FREQUENCY);

#if defined(USE_BASS)
	// create a sample
	unsigned int flags = BASS_SAMPLE_OVER_POS | BASS_SAMPLE_VAM;
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

void SoundTemplate::Reserve(size_t count)
{
	if (mSize < (mLength + count) * sizeof(short))
	{
		// reallocate
		mSize = std::max(((mLength + count) * sizeof(short) + 255) & ~255, (mSize * 3 + 1) / 2);
		mData = realloc(mData, mSize);
	}
}

void SoundTemplate::Trim(void)
{
	mSize = mLength * sizeof(short);
	mData = realloc(mData, mSize);
}

#if !defined(USE_BASS) && !defined(USE_SDL_MIXER)
static Sound *sHead;
static Sound *sTail;
static Sound *sNext;
#endif

Sound::Sound(void)
: Updatable(0)
#if !defined(USE_BASS) && !defined(USE_SDL_MIXER)
, mNext(NULL)
, mPrev(NULL)
#endif
, mSubId(0)
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
#if !defined(USE_BASS)
, mPosition(0, 0)
, mVelocity(0, 0)
#endif
#if defined(USE_SDL_MIXER)
, mPlaying(-1)
#else
, mPlaying(false)
#endif
{
	SetAction(Action(this, &Sound::Update));
}

Sound::Sound(const SoundTemplate &aTemplate, unsigned int aId, unsigned int aSubId)
: Updatable(aId)
#if !defined(USE_BASS) && !defined(USE_SDL_MIXER)
, mNext(NULL)
, mPrev(NULL)
#endif
, mSubId(aSubId)
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
#if !defined(USE_BASS)
, mPosition(0, 0)
, mVelocity(0, 0)
#endif
#if defined(USE_BASS)
, mPlaying(0)
#elif defined(USE_SDL_MIXER)
, mPlaying(-1)
#else
, mPlaying(false)
#endif
{
	// set updatable action
	SetAction(Action(this, &Sound::Update));

	// auto-play
	Play(0);
}

Sound::~Sound(void)
{
	// auto-stop
	Stop();
}


void Sound::Play(unsigned int aOffset)
{
#if defined(USE_BASS)

	// do nothing if the sound has no sample
	if (mHandle == 0)
		return;

	// if not already playing...
	if (mPlaying == 0)
	{
		// get a channel
		mPlaying = BASS_SampleGetChannel(mHandle, false);
	}

	// if no channel...
	if (mPlaying == 0)
	{
		// cannot proceed
		DebugPrint("error getting channel: %s\n", BASS_ErrorGetString());
		return;
	}

	// use normal 3d for entities, and listener-relative for non-entities
	if (!BASS_ChannelSet3DAttributes(mPlaying, Database::entity.Get(mId) ? BASS_3DMODE_NORMAL : BASS_3DMODE_RELATIVE, -1, -1, -1, -1, -1))
	{
		DebugPrint("error setting 3d attributes: %s\n", BASS_ErrorGetString());
	}

	// (re)play the channel
	if (!BASS_ChannelPlay(mPlaying, true))
	{
		DebugPrint("error playing sound: %s\n", BASS_ErrorGetString());
		return;
	}

	// if not active...
	if (!IsActive())
	{
		// pre-update to set 3D position
		Update(0.0f);

		// activate update
		Activate();
	}

#elif defined(USE_SDL_MIXER)

	// do nothing if the sound has no sample
	if (!mChunk)
		return;

	// if already playing...
	if (mPlaying >= 0)
	{
		// halt the channel
		Mix_HaltChannel(mPlaying);
	}

	// (re)play the channel
	mPlaying = Mix_PlayChannel(-1, mChunk, mRepeat);
	if (mPlaying < 0)
	{
		DebugPrint("%s\n", Mix_GetError());
		return;
	}
#elif defined(USE_SDL)
	// if not playing...
	if (!mPlaying)
	{
		// add to the active sound list
		SDL_LockAudio();
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
		if (!sNext)
			sNext = this;
		Activate();
		SDL_UnlockAudio();
	}

	// set playback position
	mOffset = aOffset;
#endif

	// if not active...
	if (!IsActive())
	{
		// pre-update to set 3D position
		Update(0.0f);

		// activate update
		Activate();
	}
}

void Sound::Stop(void)
{
#if defined(USE_BASS)
	// if playing
	if (mPlaying != 0)
	{
		// stop the channel
		BASS_ChannelStop(mPlaying);
		mPlaying = 0;
	}
#elif defined(USE_SDL_MIXER)
	// if playing...
	if (mPlaying >= 0)
	{
		// stop the channel
		Mix_HaltChannel(mPlaying);
		mPlaying = -1;
	}
#elif defined(USE_SDL)
	// if playing...
	if (mPlaying)
	{
		// remove from the active sound list
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
	// if stopped playing...
	if (BASS_ChannelIsActive(mPlaying) == BASS_ACTIVE_STOPPED)
#elif defined(USE_SDL_MIXER)
	// if stopped playing...
	if (!Mix_Playing(mPlaying))
#else
	// if stopped playing...
	if (!mRepeat && mOffset >= mLength)
#endif
	{
		// auto-delete
		Database::Typed<Sound *> &sounds = Database::sound.Open(mId);
		sounds.Delete(mSubId);
		Database::sound.Close(mId);
		delete this;
		return;
	}

#if defined(USE_BASS)
//	BASS_ChannelSetAttribute(mPlaying, BASS_ATTRIB_VOL, mVolume);

#if defined(DISTANCE_FALLOFF)
	// if attached to an entity...
	if (Entity *entity = Database::entity.Get(mId))
	{
		// update sound position
		const Vector2 &position = entity->GetPosition();
		const Vector2 &velocity = entity->GetVelocity();
		BASS_3DVECTOR pos(position.x, position.y, 0.0f);
		BASS_3DVECTOR vel(velocity.x, velocity.y, 0.0f);
		if (!BASS_ChannelSet3DPosition(mPlaying, &pos, NULL, &vel))
			DebugPrint("error setting channel 3d position: %s\n", BASS_ErrorGetString());
	}
#endif
#elif defined(USE_SDL_MIXER)
	// default to effect volume
	float volume = SOUND_VOLUME_EFFECT;
#if defined(DISTANCE_FALLOFF)
	// if named and appying rolloff...
	if (mId && SOUND_ROLLOFF_FACTOR)
	{
		// if attached to an entity...
		if (Entity *entity = Database::entity.Get(mId))
		{
			// update sound position
			mPosition = entity->GetPosition();
			mVelocity = entity->GetVelocity();
		}

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
	// update volume
	Mix_Volume(mPlaying, xs_RoundToInt(volume * MIX_MAX_VOLUME));
#else
#if defined(DISTANCE_FALLOFF)
	// if named and applying rolloff...
	if (mId && SOUND_ROLLOFF_FACTOR)
	{
		// if attached to an entity...
		if (Entity *entity = Database::entity.Get(mId))
		{
			// update sound position
			mPosition = entity->GetPosition();
			mVelocity = entity->GetVelocity();
		}
	}
#endif
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
		DebugPrint("Can't initialize device: %s", BASS_ErrorGetString());
		return;
	}

	// info header
	DebugPrint("\nBASS " BASSVERSIONTEXT " info\n");

	// get version
	DWORD version = BASS_GetVersion();
	DebugPrint("library version: %d.%d.%d.%d\n", 
		(version >> 24) & 255, 
		(version >> 16) & 255, 
		(version >> 8 ) & 255, 
		(version      ) & 255);

	// get device info
	DWORD device = BASS_GetDevice();
	BASS_DEVICEINFO deviceinfo;
	BASS_GetDeviceInfo(device, &deviceinfo);
	DebugPrint("device index: %d\n", device);
	DebugPrint("device name: %s\n", deviceinfo.name);
	DebugPrint("device driver: %s\n", deviceinfo.driver);
	DebugPrint("device flags: %08x\n", deviceinfo.flags);

	// get general info
	BASS_INFO info;
	BASS_GetInfo(&info);
	DebugPrint("capabilities: %08x\n", info.flags);
	DebugPrint("total memory: %dB\n", info.hwsize);
	DebugPrint("free memory: %dB\n", info.hwfree);
	DebugPrint("free sample slots: %d\n", info.freesam);
	DebugPrint("free 3D sample slots: %d\n", info.free3d);
	DebugPrint("minimum rate: %dHz\n", info.minrate);
	DebugPrint("maximum rate: %dHz\n", info.maxrate);
	DebugPrint("support EAX: %s\n", info.eax ? "yes" : "no");
	DebugPrint("minimum buffer: %dms\n", info.minbuf);
	DebugPrint("directsound: %d (effects %s)\n", info.dsver, info.dsver<8 ? "disabled" : "enabled");
	DebugPrint("latency: %dms\n", info.latency);
	DebugPrint("initialization: %08x\n", info.initflags);
	DebugPrint("speakers: %d\n", info.speakers);
	DebugPrint("output rate: %d\n", info.freq);

	// initialize sound volume
	UpdateSoundVolume();

#if defined(DISTANCE_FALLOFF)
	// set default 3D factors
	if (!BASS_Set3DFactors(SOUND_DISTANCE_FACTOR, SOUND_ROLLOFF_FACTOR, SOUND_DOPPLER_FACTOR))
		DebugPrint("error setting 3d factors: %s\n", BASS_ErrorGetString());
#endif

	// initialize listener
	Sound::Listener(Vector2(0, 0), Vector2(0, 0));

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
	// update listener position
	listenerpos = aPos;
	listenervel = aVel;

#if defined(USE_BASS)
	// apply position
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
	// update effect and music volumes
	BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, xs_CeilToInt(SOUND_VOLUME_EFFECT * 10000));
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, xs_CeilToInt(SOUND_VOLUME_EFFECT * 10000));
	BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, xs_CeilToInt(SOUND_VOLUME_MUSIC * 10000));
#elif defined(USE_SDL_MIXER)
	// compute music volume
	Mix_VolumeMusic(xs_CeilToInt(SOUND_VOLUME_MUSIC * MIX_MAX_VOLUME));
#endif
}

void PlaySoundCue(unsigned int aId, unsigned int aCueId)
{
	// map identifier and cue to sound
	const Database::Typed<unsigned int> &soundcues = Database::soundcue.Get(aId);
	unsigned int aSoundId = soundcues.Get(aCueId);
	const SoundTemplate &soundtemplate = Database::soundtemplate.Get(aSoundId);

	// if the sound data is valid...
	if (soundtemplate.mSize > 0)
	{
		// if a sound instance already exists...
		Database::Typed<Sound *> &sounds = Database::sound.Open(aId);
		if (Sound *s = sounds.Get(aCueId))
		{
			// retrigger
			s->Play(0);
		}
		else
		{
			// create a new instance
			sounds.Put(aCueId, new Sound(soundtemplate, aId, aCueId));
		}
	}
}

void StopSoundCue(unsigned int aId, unsigned int aCueId)
{
	// if a sound instance already exists...
	Database::Typed<Sound *> &sounds = Database::sound.Open(aId);
	if (Sound *s = sounds.Get(aCueId))
	{
		// stop and delete
		sounds.Delete(aCueId);
		delete s;
	}
	Database::sound.Close(aId);
}
