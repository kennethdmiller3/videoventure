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

bool DamagableTemplate::Configure(TiXmlElement *element)
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
	mHealth -= aDamage;
	for (Database::Typed<Listener>::Iterator itor(Database::damagablelistener.Find(id)); itor.IsValid(); ++itor)
	{
		itor.GetValue()(aSourceId, aDamage);
	}
	if (mHealth <= 0)
	{
		Entity *entity = Database::entity.Get(id);
		if (entity)
		{
			const DamagableTemplate &damagable = Database::damagabletemplate.Get(id);
			if (damagable.mSpawnOnDeath)
			{
				Database::Instantiate(damagable.mSpawnOnDeath, entity->GetAngle(), entity->GetPosition(), Vector2(0, 0));
			}
		}
		Database::Delete(id);
	}
}
