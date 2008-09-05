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
	Typed<int> hitcombo(0xa2610244 /* "hitcombo" */);

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
: mId(0), mHealth(0)
{
}

Damagable::Damagable(const DamagableTemplate &aTemplate, unsigned int aId)
: mId(aId), mHealth(aTemplate.mHealth)
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
		SetAction(Action(this, &DamagableKillUpdate::Update));
		Activate();
	}

	void Update(float aStep)
	{
		if (Damagable *damagable =Database::damagable.Get(mId))
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
	for (Database::Typed<DamageListener>::Iterator itor(Database::damagelistener.Find(mId)); itor.IsValid(); ++itor)
	{
		itor.GetValue()(mId, aSourceId, aDamage);
	}

#ifdef DEBUG_DAMAGABLE_APPLY_DAMAGE
	DebugPrint("damaged=\"%s\" source=\"%s\" owner=\"%s\" damage=%f health=%f\n",
		Database::name.Get(mId).c_str(), 
		Database::name.Get(aSourceId).c_str(),
		Database::name.Get(Database::owner.Get(aSourceId)).c_str(),
		aDamage,
		mHealth
		);
#endif

	// if destroyed...
	if (mHealth <= 0)
	{
		// set owner to source damage owner
		unsigned int aOwnerId = Database::owner.Get(aSourceId);
		Database::owner.Put(mId, aOwnerId);

		// bump the hit combo counter
		int &combo = Database::hitcombo.Open(mId);
		combo = std::max<int>(combo, Database::hitcombo.Get(aSourceId) + 1);
		Database::hitcombo.Close(mId);

		// register a kill update
		new DamagableKillUpdate(mId);

#ifdef DEBUG_DAMAGABLE_KILLED
		DebugPrint("killed=\"%s\" source=\"%s\" owner=\"%s\" combo=%d\n",
			Database::name.Get(mId).c_str(), 
			Database::name.Get(aSourceId).c_str(),
			Database::name.Get(aOwnerId).c_str(),
			combo
			);
#endif

		// notify all source kill listeners
		for (Database::Typed<KillListener>::Iterator itor(Database::killlistener.Find(aSourceId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aSourceId, mId);
		}

		// notify all owner kill listeners
		for (Database::Typed<KillListener>::Iterator itor(Database::killlistener.Find(aOwnerId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(aOwnerId, mId);
		}

		// notify all death listeners
		for (Database::Typed<DeathListener>::Iterator itor(Database::deathlistener.Find(mId)); itor.IsValid(); ++itor)
		{
			itor.GetValue()(mId, aSourceId);
		}
	}
}

void Damagable::Kill(void)
{
	// if spawn on death...
	const DamagableTemplate &damagable = Database::damagabletemplate.Get(mId);
	if (damagable.mSpawnOnDeath)
	{
#ifdef USE_CHANGE_DYNAMIC_TYPE
		// change dynamic type
		Database::Switch(mId, damagable.mSpawnOnDeath);
#else
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// instantiate the template
			Database::Instantiate(damagable.mSpawnOnDeath, Database::owner.Get(mId), mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
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
