#include "StdAfx.h"
#include "Damagable.h"
#include "Entity.h"
#include "Updatable.h"
#include "Link.h"
#include "Variable.h"

#ifdef USE_POOL_ALLOCATOR
// damagable pool
static MemoryPool sPool(sizeof(Damagable));
void *Damagable::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Damagable::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<DamagableTemplate> damagabletemplate(0x5e73241b /* "damagabletemplate" */);
	Typed<Damagable *> damagable(0x1b715375 /* "damagable" */);
	Typed<Damagable::DamageSignal > damagesignal(0x23d6dc58 /* "damagesignal" */);
	Typed<Damagable::DeathSignal > deathsignal(0x4e26c609 /* "deathsignal" */);
	Typed<Damagable::KillSignal > killsignal(0xa2bf0d7d /* "killsignal" */);
	Typed<int> hitcombo(0xa2610244 /* "hitcombo" */);

	namespace Loader
	{
		static void DamagableConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			DamagableTemplate &damagable = Database::damagabletemplate.Open(aId);
			damagable.Configure(element);
			Database::damagabletemplate.Close(aId);
		}
		Configure damagableconfigure(0x1b715375 /* "damagable" */, DamagableConfigure);
	}

	namespace Initializer
	{
		static void DamagableActivate(unsigned int aId)
		{
			const DamagableTemplate &damagabletemplate = Database::damagabletemplate.Get(aId);
			Damagable *damagable = new Damagable(damagabletemplate, aId);
			Database::damagable.Put(aId, damagable);
		}
		Activate damagableactivate(0x5e73241b /* "damagabletemplate" */, DamagableActivate);

		static void DamagableDeactivate(unsigned int aId)
		{
			if (Damagable *damagable = Database::damagable.Get(aId))
			{
				delete damagable;
				Database::damagable.Delete(aId);
				Database::damagesignal.Delete(aId);
				Database::deathsignal.Delete(aId);
				Database::killsignal.Delete(aId);
			}
		}
		Deactivate damagable(0x5e73241b /* "damagabletemplate" */, DamagableDeactivate);
	}
}


DamagableTemplate::DamagableTemplate(void)
: mHealth(0), mSpawnOnDeath(0), mSwitchOnDeath(0), mPropagateScale(0), mPropagateDeath(0)
{
}

DamagableTemplate::~DamagableTemplate(void)
{
}

bool DamagableTemplate::Configure(const tinyxml2::XMLElement *element)
{
	element->QueryFloatAttribute("health", &mHealth);
	if (const char *spawn = element->Attribute("spawnondeath"))
		mSpawnOnDeath = Hash(spawn);
	if (const char *spawn = element->Attribute("switchondeath"))
		mSwitchOnDeath = Hash(spawn);
	element->QueryFloatAttribute("propagatescale", &mPropagateScale);
	element->QueryFloatAttribute("propagatedeath", &mPropagateDeath);
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
static MemoryPool sKillPool(sizeof(DamagableKillUpdate));

void *DamagableKillUpdate::operator new(size_t aSize)
{
	return sKillPool.Alloc();
}
void DamagableKillUpdate::operator delete(void *aPtr)
{
	sKillPool.Free(aPtr);
}
#endif

void Damagable::Damage(unsigned int aSourceId, float aDamage)
{
	// ignore damage if already destroyed
	if (mHealth <= 0)
		return;

	const DamagableTemplate &damagable = Database::damagabletemplate.Get(mId);

	// update last hit time (HACK)
	Database::Typed<float> &variables = Database::variable.Open(mId);
	variables.Put(0xd62af07e /* "lasthit" */, float(sim_turn + sim_fraction) / float(sim_rate));
	Database::variable.Close(mId);

	// notify all damage listeners
	Database::damagesignal.Get(mId)(mId, aSourceId, aDamage);

	// if propagating damage...
	if (damagable.mPropagateScale)
	{
		// get backlink
		unsigned int aBackId = Database::backlink.Get(mId);

		// if the backlinked instance is damagable...
		if (Damagable *backdamagable = Database::damagable.Get(aBackId))
		{
			// propagate scaled damage
			backdamagable->Damage(aSourceId, damagable.mPropagateScale * aDamage);
		}
	}

	// deduct damage from health
	mHealth -= aDamage;

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

		// if propagating death damage...
		if (damagable.mPropagateDeath)
		{
			// get backlink
			unsigned int aBackId = Database::backlink.Get(mId);

			// if the backlinked instance is damagable...
			if (Damagable *backdamagable = Database::damagable.Get(aBackId))
			{
				// propagate death damage
				backdamagable->Damage(aSourceId, damagable.mPropagateDeath);
			}
		}

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
		Database::killsignal.Get(aSourceId)(aSourceId, mId);

		// notify all owner kill listeners
		Database::killsignal.Get(aOwnerId)(aOwnerId, mId);

		// notify all death listeners
		Database::deathsignal.Get(mId)(mId, aSourceId);
	}
}

void Damagable::Kill(void)
{
	const DamagableTemplate &damagable = Database::damagabletemplate.Get(mId);

	// if spawn on death...
	if (damagable.mSpawnOnDeath)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// instantiate the template
			unsigned int spawnId = Database::Instantiate(damagable.mSpawnOnDeath, Database::owner.Get(mId), mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());

			// propagate hit combo
			if (const int *combo = Database::hitcombo.Find(mId))
				Database::hitcombo.Put(spawnId, *combo);
		}
	}

	// if switch on death...
	if (damagable.mSwitchOnDeath)
	{
		// change dynamic type
		Database::Switch(mId, damagable.mSwitchOnDeath);
	}
	else
	{
		// delete the entity
		Database::Delete(mId);
	}
}
