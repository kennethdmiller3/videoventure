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
	float mVolume;
	float mNear;
	float mFar;
	int mRepeat;

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
	Vector2 mPosition;
	Vector2 mVelocity;
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
void PlaySoundCue(unsigned int aId, unsigned int aCueId = 0);
void StopSoundCue(unsigned int aId, unsigned int aCueId = 0);
