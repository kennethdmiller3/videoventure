#include "StdAfx.h"
#include "Damagable.h"
#include "Entity.h"
#include "Updatable.h"

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
	Typed<Typed<Damagable::DamageListener> > damagelistener(0x23d6dc58 /* "damagelistener" */);
	Typed<Typed<Damagable::DeathListener> > deathlistener(0x4e26c609 /* "deathlistener" */);
	Typed<Typed<Damagable::KillListener> > killlistener(0xa2bf0d7d /* "killlistener" */);

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
					Database::damagelistener.Delete(aId);
					Database::deathlistener.Delete(aId);
					Database::killlistener.Delete(aId);
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
}

class DamagableKillUpdate : public Updatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	DamagableKillUpdate(unsigned int aId)
		: Updatable(aId)
	{
		Activate();
	}

	void Update(float aStep)
	{
		if (Damagable *damagable =Database::damagable.Get(id))
			damagable->Kill();
		Deactivate();
		delete this;
	}
};

#ifdef USE_POOL_ALLOCATOR
// kill update pool
static boost::pool<boost::default_user_allocator_malloc_free> killpool(sizeof(DamagableKillUpdate));

void *DamagableKillUpdate::operator new(size_t aSize)
{
	return killpool.malloc();
}
void DamagableKillUpdate::operator delete(void *aPtr)
{
	killpool.free(aPtr);
}
#endif

void Damagable::Damage(unsigned int aSourceId, float aDamage)
{
	// ignore damage if already destroyed
	if (mHealth <= 0)
		return;

	// deduct damage from health
	mHealth -= aDamage;

	// notify all damage listeners
	for (Database::Typed<DamageListener>::Iterator itor(Database::damagelistener.Find(id)); itor.IsValid(); ++itor)
	{
		itor.GetValue()(id, aSourceId, aDamage);
	}

	// if destroyed...
	if (mHealth <= 0)
	{
		// register a kill update
		new DamagableKillUpdate(id);

		// notify all source kill listeners
		for (Database::Typed<KillListener>::Iterator itor(Database::killlistener.Find(aSourceId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aSourceId, id);
		}

		// notify all owner kill listeners
		unsigned int aOwnerId = Database::owner.Get(aSourceId);
		for (Database::Typed<KillListener>::Iterator itor(Database::killlistener.Find(aOwnerId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aOwnerId, id);
		}

		// notify all death listeners
		for (Database::Typed<DeathListener>::Iterator itor(Database::deathlistener.Find(id)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(id, aSourceId);
		}
	}
}

void Damagable::Kill(void)
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
			Database::Instantiate(damagable.mSpawnOnDeath, Database::owner.Get(id), entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
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
