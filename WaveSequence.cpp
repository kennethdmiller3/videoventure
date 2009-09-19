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
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(WaveSequence));
void *WaveSequence::operator new(size_t aSize)
{
	return pool.malloc();
}
void WaveSequence::operator delete(void *aPtr)
{
	pool.free(aPtr);
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
		class WaveSequenceLoader
		{
		public:
			WaveSequenceLoader()
			{
				AddConfigure(0x943e7789 /* "wavesequence" */, Entry(this, &WaveSequenceLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Open(aId);
				wavesequence.Configure(element, aId);
				Database::wavesequencetemplate.Close(aId);
			}
		}
		wavesequenceloader;
	}

	namespace Initializer
	{
		class WaveSequenceInitializer
		{
		public:
			WaveSequenceInitializer()
			{
				AddActivate(0x684715cf /* "wavesequencetemplate" */, Entry(this, &WaveSequenceInitializer::Activate));
				AddDeactivate(0x684715cf /* "wavesequencetemplate" */, Entry(this, &WaveSequenceInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const WaveSequenceTemplate &wavesequencetemplate = Database::wavesequencetemplate.Get(aId);
				WaveSequence *wavesequence = new WaveSequence(wavesequencetemplate, aId);
				Database::wavesequence.Put(aId, wavesequence);
				wavesequence->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (WaveSequence *wavesequence = Database::wavesequence.Get(aId))
				{
					delete wavesequence;
					Database::wavesequence.Delete(aId);
				}
			}
		}
		wavesequenceinitializer;
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
bool WaveEntryTemplate::Configure(const TiXmlElement *element, unsigned int aId, unsigned int aWaveId, unsigned int aEntryId)
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
}

// wave template configure
bool WaveTemplate::Configure(const TiXmlElement *element, unsigned int aId, unsigned int aWaveId)
{
	element->QueryFloatAttribute("predelay", &mPreDelay);
	element->QueryFloatAttribute("postdelay", &mPostDelay);

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xcac3d793 /* "entry" */:
			if (const char *name = child->Attribute("name"))
			{
				unsigned int aEntryId = Hash(name);
				WaveEntryTemplate &entry = mEntries.Open(aEntryId);
				entry.Configure(child, aId, aWaveId, aEntryId);
				mEntries.Close(aEntryId);
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
}

// wavesequence template destructor
WaveSequenceTemplate::~WaveSequenceTemplate(void)
{
}

// wavesequence template configure
bool WaveSequenceTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("predelay", &mPreDelay);
	element->QueryFloatAttribute("postdelay", &mPostDelay);

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xa9f017d4 /* "wave" */:
			if (const char *name = child->Attribute("name"))
			{
				unsigned int aWaveId = Hash(name);
				WaveTemplate &wave = mWaves.Open(aWaveId);
				wave.Configure(child, aId, aWaveId);
				mWaves.Close(aWaveId);
			}
			break;

		case 0xfe9c11ec /* "restart" */:
			if (const char *name = child->Attribute("name"))
			{
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
	DebugPrint("%08x sequence enter\n", mId);

	mWaveIndex = 0;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequenceStartPreDelay));
	Activate();
}

void WaveSequence::SequenceStartPreDelay(float aStep)
{
	DebugPrint("%08x sequence start pre-delay\n", mId);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	mTimer -= wavesequence.mPreDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequencePreDelay));
	Activate();
}

void WaveSequence::SequencePreDelay(float aStep)
{
	DebugPrint("%08x sequence pre-delay\n", mId);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	mTimer -= aStep;
	Deactivate();
	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	if (waveitor.IsValid())
		SetAction(Action(this, &WaveSequence::WaveEnter));
	else
		SetAction(Action(this, &WaveSequence::SequencePostDelay));
	Activate();
}

void WaveSequence::WaveEnter(float aStep)
{
	DebugPrint("%08x wave %d enter\n", mId, mWaveIndex);

	mEntryIndex = 0;

	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveStartPreDelay));
	Activate();
}

void WaveSequence::WaveStartPreDelay(float aStep)
{
	DebugPrint("%08x wave %d start pre-delay\n", mId, mWaveIndex);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	mTimer -= waveitor.GetValue().mPreDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::WavePreDelay));
	Activate();
}

void WaveSequence::WavePreDelay(float aStep)
{
	DebugPrint("%08x wave %d pre-delay\n", mId, mWaveIndex);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	mTimer -= aStep;
	Deactivate();
	Database::Typed<WaveEntryTemplate>::Iterator entryitor(&waveitor.GetValue().mEntries, mEntryIndex);
	if (entryitor.IsValid())
		SetAction(Action(this, &WaveSequence::WaveSpawn));
	else
		SetAction(Action(this, &WaveSequence::WaveStartPostDelay));
	Activate();
}

void WaveSequence::WaveSpawn(float aStep)
{
	DebugPrint("%08x wave %d spawn\n", mId, mWaveIndex);

	mTimer += aStep;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	Database::Typed<WaveEntryTemplate>::Iterator entryitor(&waveitor.GetValue().mEntries, mEntryIndex);
	while (entryitor.IsValid())
	{
		// get the entry
		const WaveEntryTemplate &entry = entryitor.GetValue();

		// assume the list is sorted :D
		if (mTimer < entry.mTime)
		{
			return;
		}

		DebugPrint("%08x wave %d entry %d trigger\n", mId, mWaveIndex, mEntryIndex);

		// spawn the entry
		Entity *entity = Database::entity.Get(mId);
		if (unsigned int spawnId = entry.mSpawn.Instantiate(mId,
			entity->GetTransform(), Transform2(entity->GetOmega(), entity->GetVelocity())))
		{
			// add a tracker
			Database::wavesequencetracker.Put(spawnId, WaveSequenceTracker(mId));
		}

		++entryitor;
		mEntryIndex = entryitor.GetSlot();
	}

	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveTrack));
	Activate();
}

void WaveSequence::WaveTrack(float aStep)
{
	DebugPrint("%08x wave %d track\n", mId, mWaveIndex);
	if (mTrack > 0)
		return;

	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveStartPostDelay));
	Activate();
}

void WaveSequence::WaveStartPostDelay(float aStep)
{
	DebugPrint("%08x wave %d start post-delay\n", mId, mWaveIndex);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	mTimer -= waveitor.GetValue().mPostDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::WavePostDelay));
	Activate();
}

void WaveSequence::WavePostDelay(float aStep)
{
	DebugPrint("%08x wave %d post-delay\n", mId, mWaveIndex);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	mTimer -= aStep;
	Deactivate();
	SetAction(Action(this, &WaveSequence::WaveExit));
	Activate();
}

void WaveSequence::WaveExit(float aStep)
{
	DebugPrint("%08x wave %d exit\n", mId, mWaveIndex);

	++mWaveIndex;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Deactivate();
	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	if (waveitor.IsValid())
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
	for (Database::Typed<WaveTemplate>::Iterator itor(&wavesequence.mWaves); itor.IsValid(); ++itor)
	{
		if (itor.GetKey() == wavesequence.mRestart)
		{
			mWaveIndex = itor.GetSlot();
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
	DebugPrint("%08x sequence start post-delay\n", mId);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	mTimer -= wavesequence.mPostDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequencePostDelay));
	Activate();
}

void WaveSequence::SequencePostDelay(float aStep)
{
	DebugPrint("%08x sequence post-delay\n", mId);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	mTimer -= aStep;
	Deactivate();
	SetAction(Action(this, &WaveSequence::SequenceExit));
	Activate();
}

void WaveSequence::SequenceExit(float aStep)
{
	DebugPrint("%08x sequence exit\n", mId);

	Deactivate();
}
