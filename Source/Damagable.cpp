#include "StdAfx.h"
#include "Damagable.h"
#include "Entity.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// damagable pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Damagable));
void *Damagable::operator new(size_t aSize)
{
	return pool.malloc();
}
void Damagable::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<DamagableTemplate> damagabletemplate(0x5e73241b /* "damagabletemplate" */);
	Typed<Damagable *> damagable(0x1b715375 /* "damagable" */);
	Typed<Typed<Damagable::Listener> > damagablelistener(0x1e01f5e1 /* "damagablelistener" */);

	namespace Loader
	{
		class DamagableLoader
		{
		public:
			DamagableLoader()
			{
				AddConfigure(0x1b715375 /* "damagable" */, Entry(this, &DamagableLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				DamagableTemplate &damagable = Database::damagabletemplate.Open(aId);
				damagable.Configure(element);
				Database::damagabletemplate.Close(aId);
			}
		}
		damagableloader;
	}

	namespace Initializer
	{
		class DamagableInitializer
		{
		public:
			DamagableInitializer()
			{
				AddActivate(0x5e73241b /* "damagabletemplate" */, Entry(this, &DamagableInitializer::Activate));
				AddDeactivate(0x5e73241b /* "damagabletemplate" */, Entry(this, &DamagableInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const DamagableTemplate &damagabletemplate = Database::damagabletemplate.Get(aId);
				Damagable *damagable = new Damagable(damagabletemplate, aId);
				Database::damagable.Put(aId, damagable);
			}

			void Deactivate(unsigned int aId)
			{
				if (Damagable *damagable = Database::damagable.Get(aId))
				{
					delete damagable;
					Database::damagable.Delete(aId);
					Database::damagablelistener.Delete(aId);
				}
			}
		}
		damagableinitializer;
	}
}


DamagableTemplate::DamagableTemplate(void)
: mHealth(0), mSpawnOnDeath(0)
{
}

DamagableTemplate::~DamagableTemplate(void)
{
}

bool DamagableTemplate::Configure(const TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x1b715375 /* "damagable" */)
		return false;

	element->QueryFloatAttribute("health", &mHealth);
	if (const char *spawn = element->Attribute("spawnondeath"))
		mSpawnOnDeath = Hash(spawn);
	return true;
}


Damagable::Damagable(void)
: id(0), mHealth(0)
{
}

Damagable::Damagable(const DamagableTemplate &aTemplate, unsigned int aId)
: id(aId), mHealth(aTemplate.mHealth)
{
}

Damagable::~Damagable(void)
{
	Database::damagablelistener.Delete(id);
}

void Damagable::Damage(unsigned int aSourceId, float aDamage)
{
	// ignore damage if already destroyed
	if (mHealth <= 0)
		return;

	// deduct damage from health
	mHealth -= aDamage;

	// notify all damagable listeners
	for (Database::Typed<Listener>::Iterator itor(Database::damagablelistener.Find(id)); itor.IsValid(); ++itor)
	{
		itor.GetValue()(aSourceId, aDamage);
	}

	// if destroyed...
	if (mHealth <= 0)
	{
		// if spawn on death...
		const DamagableTemplate &damagable = Database::damagabletemplate.Get(id);
		if (damagable.mSpawnOnDeath)
		{
#ifdef USE_CHANGE_DYNAMIC_TYPE
			// change dynamic type
			Database::Deactivate(id);
			Database::parent.Put(id, damagable.mSpawnOnDeath);
			Database::Activate(id);
#else
			// get the entity
			Entity *entity = Database::entity.Get(id);
			if (entity)
			{
				// instantiate the template
				Database::Instantiate(damagable.mSpawnOnDeath, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity());
			}
#endif
		}
#ifdef USE_CHANGE_DYNAMIC_TYPE
		else
#endif
		{
			// delete the entity
			Database::Delete(id);
		}
	}
}
