#pragma once

#include "Updatable.h"

class SoundTemplate
{
public:
	unsigned char *mData;
	unsigned int mLength;
	float mVolume;

public:
	SoundTemplate(void);
	SoundTemplate(const SoundTemplate &aTemplate);
	~SoundTemplate(void);
};

class Sound
	: public Updatable
{
public:
    unsigned char *mData;
	unsigned int mLength;
    unsigned int mOffset;
	float mVolume;
	unsigned int mRepeat;
	Vector2 mPosition;

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

	// update
	void Update(float aStep);
};

namespace Database
{
	extern Typed<SoundTemplate> soundtemplate;
	extern Typed<Sound *> sound;
}

void PlaySound(unsigned int aId);
