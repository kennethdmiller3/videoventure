#include "StdAfx.h"
#include "Capturable.h"
#include "Entity.h"
#include "Updatable.h"

#ifdef USE_POOL_ALLOCATOR
// capturable pool
static MemoryPool sPool(sizeof(Capturable));
void *Capturable::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Capturable::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<CapturableTemplate> capturabletemplate(0xc19fcdd4 /* "capturabletemplate" */);
	Typed<Capturable *> capturable(0xec2b4992 /* "capturable" */);
	Typed<Typed<Capturable::CaptureSignal> > capturesignal(0x672ec99d /* "capturesignal" */);

	namespace Loader
	{
		class CapturableLoader
		{
		public:
			CapturableLoader()
			{
				AddConfigure(0xec2b4992 /* "capturable" */, Entry(this, &CapturableLoader::Configure));
			}

			~CapturableLoader()
			{
				RemoveConfigure(0xec2b4992 /* "capturable" */, Entry(this, &CapturableLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				CapturableTemplate &capturable = Database::capturabletemplate.Open(aId);
				capturable.Configure(element);
				Database::capturabletemplate.Close(aId);
			}
		}
		capturableloader;
	}

	namespace Initializer
	{
		class CapturableInitializer
		{
		public:
			CapturableInitializer()
			{
				AddActivate(0xc19fcdd4 /* "capturabletemplate" */, Entry(this, &CapturableInitializer::Activate));
				AddDeactivate(0xc19fcdd4 /* "capturabletemplate" */, Entry(this, &CapturableInitializer::Deactivate));
			}

			~CapturableInitializer()
			{
				RemoveActivate(0xc19fcdd4 /* "capturabletemplate" */, Entry(this, &CapturableInitializer::Activate));
				RemoveDeactivate(0xc19fcdd4 /* "capturabletemplate" */, Entry(this, &CapturableInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const CapturableTemplate &capturabletemplate = Database::capturabletemplate.Get(aId);
				Capturable *capturable = new Capturable(capturabletemplate, aId);
				Database::capturable.Put(aId, capturable);
			}

			void Deactivate(unsigned int aId)
			{
				if (Capturable *capturable = Database::capturable.Get(aId))
				{
					delete capturable;
					Database::capturable.Delete(aId);
					Database::capturesignal.Delete(aId);
				}
			}
		}
		capturableinitializer;
	}
}


CapturableTemplate::CapturableTemplate(void)
: mResistance(0)
, mSpawnOnCapture(0)
, mSwitchOnCapture(0)
{
}

CapturableTemplate::~CapturableTemplate(void)
{
}

bool CapturableTemplate::Configure(const tinyxml2::XMLElement *element)
{
	element->QueryFloatAttribute("resistance", &mResistance);
	if (const char *spawn = element->Attribute("spawnoncapture"))
		mSpawnOnCapture = Hash(spawn);
	if (const char *spawn = element->Attribute("switchoncapture"))
		mSwitchOnCapture = Hash(spawn);
	return true;
}


Capturable::Capturable(void)
: mId(0), mResistance(0)
{
}

Capturable::Capturable(const CapturableTemplate &aTemplate, unsigned int aId)
: mId(aId), mResistance(aTemplate.mResistance)
{
}

Capturable::~Capturable(void)
{
}

class CapturableCaptureUpdate : public Updatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	CapturableCaptureUpdate(unsigned int aId)
		: Updatable(aId)
	{
		SetAction(Action(this, &CapturableCaptureUpdate::Update));
		Activate();
	}

	void Update(float aStep)
	{
		if (Capturable *capturable =Database::capturable.Get(mId))
			capturable->Capture();
		Deactivate();
		delete this;
	}
};

#ifdef USE_POOL_ALLOCATOR
// capture update pool
static MemoryPool sCapturePool(sizeof(CapturableCaptureUpdate));

void *CapturableCaptureUpdate::operator new(size_t aSize)
{
	return sCapturePool.Alloc();
}
void CapturableCaptureUpdate::operator delete(void *aPtr)
{
	sCapturePool.Free(aPtr);
}
#endif

void Capturable::Persuade(unsigned int aSourceId, float aEffect)
{
	// ignore effect if already captured
	if (mResistance <= 0)
		return;

	// deduct effect from resistance
	mResistance -= aEffect;

	// if captured...
	if (mResistance <= 0)
	{
		// register a capture update
		new CapturableCaptureUpdate(mId);

		// notify all capture signals
		for (Database::Typed<CaptureSignal>::Iterator itor(Database::capturesignal.Find(aSourceId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aSourceId, mId);
		}

		// notify all owner capture signals
		unsigned int aOwnerId = Database::owner.Get(aSourceId);
		for (Database::Typed<CaptureSignal>::Iterator itor(Database::capturesignal.Find(aOwnerId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aOwnerId, mId);
		}
	}
}

void Capturable::Capture(void)
{
	const CapturableTemplate &capturable = Database::capturabletemplate.Get(mId);

	// if spawning on capture...
	if (capturable.mSpawnOnCapture)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// instantiate the template
			Database::Instantiate(capturable.mSpawnOnCapture, Database::owner.Get(mId), mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
		}
	}

	// if switching on capture...
	if (capturable.mSwitchOnCapture)
	{
		// change dynamic type
		Database::Switch(mId, capturable.mSwitchOnCapture);
	}
	else
	{
		// delete the entity
		Database::Delete(mId);
	}
}
