#pragma once

#include "Updatable.h"

#if defined(USE_SDL_MIXER)
extern "C" struct Mix_Chunk;
#endif

class SoundTemplate
{
public:
	void *mData;		// data buffer
	size_t mSize;		// data buffer size in bytes
	size_t mLength;		// sound length
	float mVolume;		// intrinsic volume
	float mNear;		// maximum volume closer than this
	float mFar;			// minimum volume further than this
	int mRepeat;		// repeat count

#if defined(USE_BASS)
	int mFrequency;
	unsigned long mHandle;
#elif defined(USE_SDL_MIXER)
	Mix_Chunk *mChunk;
#endif

public:
	SoundTemplate(void);
	SoundTemplate(const SoundTemplate &aTemplate);
	~SoundTemplate(void);

	bool Configure(const TiXmlElement *element, unsigned int id);

	void Reserve(size_t count);
	void Trim(void);

	inline void Append(short value)
	{
		// append sample
		static_cast<short *>(mData)[mLength++] = value;
	}

	inline void Append(short value, size_t count)
	{
		// append samples
		for (size_t i = 0; i < count; ++i)
		{
			static_cast<short *>(mData)[mLength++] = value;
		}
	}

	inline void Append(short *data, size_t count)
	{
		// append data
		memcpy(static_cast<short *>(mData) + mLength, data, count * sizeof(short));
		mLength += count;
	}
};

class Sound
	: public Updatable
{
private:
	// linked list
	Sound *mNext;
	Sound *mPrev;

public:
#if defined(USE_BASS)
	unsigned long mHandle;
#elif defined(USE_SDL_MIXER)
	Mix_Chunk *mChunk;
	float mNear;
	float mFar;
#elif defined(USE_SDL)
    void *mData;
	size_t mLength;
    size_t mOffset;
#endif
	int mRepeat;
	float mVolume;
#if !defined(USE_BASS)
	Vector2 mPosition;
	Vector2 mVelocity;
#endif
	int mPlaying;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// default constructor
	Sound(void);

	// constructor
	Sound(const SoundTemplate &aTemplate, unsigned int aId);

	// destructor
	~Sound(void);

	// play
	void Play(unsigned int mOffset = 0);
	void Stop(void);

	// is playing?
	bool IsPlaying(void)
	{
		return mRepeat > 0;
	}

	// update
	void Update(float aStep);

private:
	static Vector2 listenerpos;
	static Vector2 listenervel;

public:
	// initialize
	static void Init(void);

	// deinitialize
	static void Done(void);

	// pause
	static void Pause(void);

	// resume
	static void Resume(void);

	// set listener position
	static void Listener(Vector2 aPos, Vector2 aVel = Vector2(0, 0));

	// software mixer
	friend void MixSound(void *userdata, unsigned char *stream, int len);
};

namespace Database
{
	extern Typed<SoundTemplate> soundtemplate;
	extern Typed<Typed<unsigned int> > soundcue;
	extern Typed<Typed<Sound *> > sound;
}

// update sound volume
void UpdateSoundVolume(void);

// play/stop sound cues
void GAME_API PlaySoundCue(unsigned int aId, unsigned int aCueId = 0);
void GAME_API StopSoundCue(unsigned int aId, unsigned int aCueId = 0);
