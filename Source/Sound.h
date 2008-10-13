#pragma once

#include "Updatable.h"

class SoundTemplate
{
public:
	short *mData;		// data buffer
	size_t mSize;		// data buffer size in bytes
	size_t mLength;		// sound length
	float mVolume;
	int mRepeat;

public:
	SoundTemplate(void);
	SoundTemplate(const SoundTemplate &aTemplate);
	~SoundTemplate(void);

	bool ConfigureFile(const TiXmlElement *element, unsigned int id);
	bool ConfigureSample(const TiXmlElement *element, unsigned int id);
	bool ConfigurePokey(const TiXmlElement *element, unsigned int id);
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
    short *mData;
	size_t mLength;
    size_t mOffset;
	float mVolume;
	int mRepeat;
	Vector2 mPosition;
	bool mPlaying;

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

public:
	static Vector2 listenerpos;

	// mix
	static void Mix(void *userdata, unsigned char *stream, int len);
};

namespace Database
{
	extern Typed<SoundTemplate> soundtemplate;
	extern Typed<Typed<unsigned int> > soundcue;
	extern Typed<Typed<Sound *> > sound;
}

void PlaySoundCue(unsigned int aId, unsigned int aCueId = 0);
void StopSoundCue(unsigned int aId, unsigned int aCueId = 0);
