#include "StdAfx.h"
#include "Capturable.h"
#include "Entity.h"
#include "Updatable.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// capturable pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Capturable));
void *Capturable::operator new(size_t aSize)
{
	return pool.malloc();
}
void Capturable::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<CapturableTemplate> capturabletemplate(0xc19fcdd4 /* "capturabletemplate" */);
	Typed<Capturable *> capturable(0xec2b4992 /* "capturable" */);
	Typed<Typed<Capturable::CaptureListener> > capturelistener(0x520b0775 /* "capturelistener" */);

	namespace Loader
	{
		class CapturableLoader
		{
		public:
			CapturableLoader()
			{
				AddConfigure(0xec2b4992 /* "capturable" */, Entry(this, &CapturableLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
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
					Database::capturelistener.Delete(aId);
				}
			}
		}
		capturableinitializer;
	}
}


CapturableTemplate::CapturableTemplate(void)
: mResistance(0), mSpawnOnCapture(0)
{
}

CapturableTemplate::~CapturableTemplate(void)
{
}

bool CapturableTemplate::Configure(const TiXmlElement *element)
{
	element->QueryFloatAttribute("resistance", &mResistance);
	if (const char *spawn = element->Attribute("spawnoncapture"))
		mSpawnOnCapture = Hash(spawn);
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
static boost::pool<boost::default_user_allocator_malloc_free> capturepool(sizeof(CapturableCaptureUpdate));

void *CapturableCaptureUpdate::operator new(size_t aSize)
{
	return capturepool.malloc();
}
void CapturableCaptureUpdate::operator delete(void *aPtr)
{
	capturepool.free(aPtr);
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

		// notify all capture listeners
		for (Database::Typed<CaptureListener>::Iterator itor(Database::capturelistener.Find(aSourceId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aSourceId, mId);
		}

		// notify all owner capture listeners
		unsigned int aOwnerId = Database::owner.Get(aSourceId);
		for (Database::Typed<CaptureListener>::Iterator itor(Database::capturelistener.Find(aOwnerId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aOwnerId, mId);
		}
	}
}

void Capturable::Capture(void)
{
	// if spawn on capture...
	const CapturableTemplate &capturable = Database::capturabletemplate.Get(mId);
	if (capturable.mSpawnOnCapture)
	{
#ifdef USE_CHANGE_DYNAMIC_TYPE
		// change dynamic type
		Database::Switch(mId, capturable.mSpawnOnCapture);
#else
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// instantiate the template
			Database::Instantiate(capturable.mSpawnOnCapture, Database::owner.Get(mId), mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
		}
#endif
	}
#ifdef USE_CHANGE_DYNAMIC_TYPE
	else
#endif
	{
		// delete the entity
		Database::Delete(mId);
	}
}
