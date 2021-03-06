#include "StdAfx.h"
#include "WaveSequence.h"
#include "Entity.h"
#include "Renderable.h"
#include "Team.h"
#include "Spawn.h"

/*
WaveSequence
{
	Pre-Sequence Delay
	Wave[]
	{
		Pre-Wave Delay
		Spawn Entities
		Track Entities
		Post-Wave Delay
	}
	Post-Sequence Delay
}
*/

#ifdef USE_POOL_ALLOCATOR
// wavesequence pool
static MemoryPool sPool(sizeof(WaveSequence));
void *WaveSequence::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void WaveSequence::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


class WaveSequenceTracker
{
public:
	unsigned int mId;

	WaveSequenceTracker(unsigned int aId = 0)
		: mId(aId)
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(1);
	}

	WaveSequenceTracker(const WaveSequenceTracker &aSource)
		: mId(aSource.mId)
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(1);
	}

	~WaveSequenceTracker()
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(-1);
	}

	const WaveSequenceTracker &operator=(const WaveSequenceTracker &aSource)
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(-1);
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(1);
		return *this;
	}
};


namespace Database
{
	Typed<WaveSequenceTemplate> wavesequencetemplate(0x684715cf /* "wavesequencetemplate" */);
	Typed<WaveSequence *> wavesequence(0x943e7789 /* "wavesequence" */);
	Typed<WaveSequenceTracker> wavesequencetracker(0xfebb6857 /* "wavesequencetracker" */);

	namespace Loader
	{
		static void WaveSequenceConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Open(aId);
			wavesequence.Configure(element, aId);
			Database::wavesequencetemplate.Close(aId);
		}
		Configure wavesequenceconfigure(0x943e7789 /* "wavesequence" */, WaveSequenceConfigure);
	}

	namespace Initializer
	{
		static void WaveSequenceActivate(unsigned int aId)
		{
			const WaveSequenceTemplate &wavesequencetemplate = Database::wavesequencetemplate.Get(aId);
			WaveSequence *wavesequence = new WaveSequence(wavesequencetemplate, aId);
			Database::wavesequence.Put(aId, wavesequence);
			wavesequence->Activate();
		}
		Activate wavesequenceactivate(0x684715cf /* "wavesequencetemplate" */, WaveSequenceActivate);

		static void WaveSequenceDeactivate(unsigned int aId)
		{
			if (WaveSequence *wavesequence = Database::wavesequence.Get(aId))
			{
				delete wavesequence;
				Database::wavesequence.Delete(aId);
			}
		}
		Deactivate wavesequencedeactivate(0x684715cf /* "wavesequencetemplate" */, WaveSequenceDeactivate);
	}
}


// WAVE ENTRY

// wave entry constructor
WaveEntryTemplate::WaveEntryTemplate(void)
: mTime(0.0f)
, mSpawn()
{
}

// wave entry template configure
bool WaveEntryTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId, unsigned int aWaveId, unsigned int aEntryId)
{
	element->QueryFloatAttribute("time", &mTime);
	mSpawn.Configure(element, aId);
	return true;
}


// WAVE

// wave template constructor
WaveTemplate::WaveTemplate(void)
: mPreDelay(0.0f)
, mPostDelay(0.0f)
{
	mEntries = NULL;
	mEntryCount = 0;
}

// wave template destructor
WaveTemplate::~WaveTemplate(void)
{
	delete[] mEntries;
}

// wave template configure
bool WaveTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId, unsigned int aWaveId)
{
	mWaveId = aWaveId;

	element->QueryFloatAttribute("predelay", &mPreDelay);
	element->QueryFloatAttribute("postdelay", &mPostDelay);

	// count entries
	mEntryCount = 0;
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xcac3d793 /* "entry" */:
			++mEntryCount;
		}
	}
	mEntries = new WaveEntryTemplate[mEntryCount];
	mEntryCount = 0;

	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xcac3d793 /* "entry" */:
			{
				const char *name = child->Attribute("name");
				unsigned int aEntryId = Hash(name);
				WaveEntryTemplate &entry = mEntries[mEntryCount++];
				entry.Configure(child, aId, aWaveId, aEntryId);
			}
			break;
		}
	}
	return true;
}


// WAVE SEQUENCE

// wavesequence template constructor
WaveSequenceTemplate::WaveSequenceTemplate(void)
{
	mWaves = NULL;
	mWaveCount = 0;
}

// wavesequence template destructor
WaveSequenceTemplate::~WaveSequenceTemplate(void)
{
	delete[] mWaves;
}

// wavesequence template configure
bool WaveSequenceTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("predelay", &mPreDelay);
	element->QueryFloatAttribute("postdelay", &mPostDelay);

	// count entries
	mWaveCount = 0;
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xa9f017d4 /* "wave" */:
			++mWaveCount;
		}
	}
	mWaves = new WaveTemplate[mWaveCount];
	mWaveCount = 0;

	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xa9f017d4 /* "wave" */:
			{
				const char *name = child->Attribute("name");
				unsigned int aWaveId = Hash(name);
				WaveTemplate &wave = mWaves[mWaveCount++];
				wave.Configure(child, aId, aWaveId);
			}
			break;

		case 0xfe9c11ec /* "restart" */:
			{
				const char *name = child->Attribute("name");
				mRestart = Hash(name);
			}
			break;
		}
	}

	return true;
}


// wavesequence default constructor
WaveSequence::WaveSequence(void)
: Updatable(0)
, mTrack(0)
, mTimer(0)
{
}

// wavesequence instantiation constructor
WaveSequence::WaveSequence(const WaveSequenceTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mEntryIndex(0)
, mWaveIndex(-1)
, mTrack(0)
, mTimer(0)
{
	SetAction(Action(this, &WaveSequence::SequenceEnter));
}

// wavesequence destructor
WaveSequence::~WaveSequence(void)
{
	// remove listeners
}

void WaveSequence::SequenceEnter(float aStep)
{
	DebugPrint("\"%s\" sequence enter\n", Database::name.Get(mId).c_str());

	mWaveIndex = 0;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequenceStartPreDelay));
	Activate();
}

void WaveSequence::SequenceStartPreDelay(float aStep)
{
	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	DebugPrint("\"%s\" sequence start pre-delay: %.2fs\n", Database::name.Get(mId).c_str(), wavesequence.mPreDelay);

	mTimer -= wavesequence.mPreDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequencePreDelay));
	Activate();
}

void WaveSequence::SequencePreDelay(float aStep)
{
	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	DebugPrint("\"%s\" sequence finish pre-delay\n", Database::name.Get(mId).c_str());

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	mTimer -= aStep;
	Deactivate();
	if (mWaveIndex < wavesequence.mWaveCount)
		SetAction(Action(this, &WaveSequence::WaveEnter));
	else
		SetAction(Action(this, &WaveSequence::SequencePostDelay));
	Activate();
}

void WaveSequence::WaveEnter(float aStep)
{
	DebugPrint("\"%s\" wave %d enter\n", Database::name.Get(mId).c_str(), mWaveIndex);

	mEntryIndex = 0;

	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveStartPreDelay));
	Activate();
}

void WaveSequence::WaveStartPreDelay(float aStep)
{
	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	const WaveTemplate &wave = wavesequence.mWaves[mWaveIndex];

	mTimer -= wave.mPreDelay;

	DebugPrint("\"%s\" wave %d start pre-delay: %.2fs\n", Database::name.Get(mId).c_str(), mWaveIndex, wave.mPreDelay);

	Deactivate();
	SetAction(Action(this, &WaveSequence::WavePreDelay));
	Activate();
}

void WaveSequence::WavePreDelay(float aStep)
{
	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	DebugPrint("\"%s\" wave %d finish pre-delay\n", Database::name.Get(mId).c_str(), mWaveIndex);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	const WaveTemplate &wave = wavesequence.mWaves[mWaveIndex];

	mTimer -= aStep;
	Deactivate();
	if (mEntryIndex < wave.mEntryCount)
		SetAction(Action(this, &WaveSequence::WaveSpawn));
	else
		SetAction(Action(this, &WaveSequence::WaveStartPostDelay));
	Activate();
}

void WaveSequence::WaveSpawn(float aStep)
{
	DebugPrint("\"%s\" wave %d spawn\n", Database::name.Get(mId).c_str(), mWaveIndex);

	mTimer += aStep;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	const WaveTemplate &wave = wavesequence.mWaves[mWaveIndex];

	while (mEntryIndex < wave.mEntryCount)
	{
		// get the entry
		const WaveEntryTemplate &entry = wave.mEntries[mEntryIndex];

		// assume the list is sorted :D
		if (mTimer < entry.mTime)
		{
			return;
		}

		DebugPrint("\"%s\" wave %d entry %d trigger\n", Database::name.Get(mId).c_str(), mWaveIndex, mEntryIndex);

		// spawn the entry
		Entity *entity = Database::entity.Get(mId);
		unsigned int spawnId = entry.mSpawn.Instantiate(mId,
			entity->GetTransform(), Transform2(entity->GetOmega(), entity->GetVelocity()));

		// add a tracker
		Database::wavesequencetracker.Put(spawnId, WaveSequenceTracker(mId));

		++mEntryIndex;
	}

	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveTrack));
	Activate();
}

void WaveSequence::WaveTrack(float aStep)
{
	if (mTrack > 0)
		return;

	DebugPrint("\"%s\" wave %d finish track\n", Database::name.Get(mId).c_str(), mWaveIndex);

	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveStartPostDelay));
	Activate();
}

void WaveSequence::WaveStartPostDelay(float aStep)
{
	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	const WaveTemplate &wave = wavesequence.mWaves[mWaveIndex];

	mTimer -= wave.mPostDelay;

	DebugPrint("\"%s\" wave %d start post-delay: %.2fs\n", Database::name.Get(mId).c_str(), mWaveIndex, wave.mPostDelay);

	Deactivate();
	SetAction(Action(this, &WaveSequence::WavePostDelay));
	Activate();
}

void WaveSequence::WavePostDelay(float aStep)
{
	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	DebugPrint("\"%s\" wave %d finish post-delay\n", Database::name.Get(mId).c_str(), mWaveIndex);

	mTimer -= aStep;
	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveExit));
	Activate();
}

void WaveSequence::WaveExit(float aStep)
{
	DebugPrint("\"%s\" wave %d exit\n", Database::name.Get(mId).c_str(), mWaveIndex);

	++mWaveIndex;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Deactivate();
	if (mWaveIndex < wavesequence.mWaveCount)
		SetAction(Action(this, &WaveSequence::WaveEnter));
	else if (wavesequence.mRestart)
		SetAction(Action(this, &WaveSequence::WaveRestart));
	else
		SetAction(Action(this, &WaveSequence::SequenceStartPostDelay));
	Activate();
}

void WaveSequence::WaveRestart(float aStep)
{
	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);
	for (mWaveIndex = 0; mWaveIndex < wavesequence.mWaveCount; ++mWaveIndex)
	{
		if (wavesequence.mWaves[mWaveIndex].mWaveId == wavesequence.mRestart)
		{
			Deactivate();
			SetAction(Action(this, &WaveSequence::WaveEnter));
			Activate();
			return;
		}
	}

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequenceStartPostDelay));
	Activate();
}

void WaveSequence::SequenceStartPostDelay(float aStep)
{
	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	DebugPrint("\"%s\" sequence start post-delay: %.2fs\n", Database::name.Get(mId).c_str(), wavesequence.mPostDelay);

	mTimer -= wavesequence.mPostDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequencePostDelay));
	Activate();
}

void WaveSequence::SequencePostDelay(float aStep)
{
	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	DebugPrint("\"%s\" sequence finish post-delay\n", Database::name.Get(mId).c_str());

	mTimer -= aStep;
	Deactivate();
	SetAction(Action(this, &WaveSequence::SequenceExit));
	Activate();
}

void WaveSequence::SequenceExit(float aStep)
{
	DebugPrint("\"%s\" sequence exit\n", Database::name.Get(mId).c_str());

	Deactivate();
}
