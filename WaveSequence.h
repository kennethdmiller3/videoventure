#pragma once

#include "Updatable.h"
#include "Spawn.h"

class WaveEntryTemplate
{
public:
	float mTime;
	SpawnTemplate mSpawn;

public:
	WaveEntryTemplate(void);

	bool Configure(const TiXmlElement *element, unsigned int aId, unsigned int aWaveId, unsigned int aEntryId);
};

class WaveTemplate
{
public:
	float mPreDelay;
	Database::Typed<WaveEntryTemplate> mEntries;
	float mPostDelay;

public:
	WaveTemplate(void);

	bool Configure(const TiXmlElement *element, unsigned int aId, unsigned int aWaveId);
};

class WaveSequenceTemplate
{
public:
	float mPreDelay;
	Database::Typed<WaveTemplate> mWaves;
	unsigned int mRestart;
	float mPostDelay;

public:
	WaveSequenceTemplate(void);
	~WaveSequenceTemplate(void);

	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class WaveSequence
	: public Updatable
{
protected:
	int mEntryIndex;
	int mWaveIndex;
	int mTrack;
	float mTimer;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	WaveSequence(void);
	WaveSequence(const WaveSequenceTemplate &aTemplate, unsigned int aId);
	virtual ~WaveSequence(void);

	// state machine
	void SequenceEnter(float aStep);
	void SequenceStartPreDelay(float aStep);
	void SequencePreDelay(float aStep);
	void WaveEnter(float aStep);
	void WaveStartPreDelay(float aStep);
	void WavePreDelay(float aStep);
	void WaveSpawn(float aStep);
	void WaveTrack(float aStep);
	void WaveStartPostDelay(float aStep);
	void WavePostDelay(float aStep);
	void WaveExit(float aStep);
	void WaveRestart(float aStep);
	void SequenceStartPostDelay(float aStep);
	void SequencePostDelay(float aStep);
	void SequenceExit(float aStep);

protected:
	friend class WaveSequenceTracker;

	// tracking
	void Track(int aAdd)
	{
		mTrack += aAdd;
	}
};

namespace Database
{
	extern Typed<WaveSequenceTemplate> wavesequencetemplate;
	extern Typed<WaveSequence *> wavesequence;
}
