#include "StdAfx.h"
#include "Damagable.h"
#include "Entity.h"

namespace Database
{
	Typed<DamagableTemplate> damagabletemplate("damagabletemplate");
	Typed<Damagable *> damagable("damagable");
	Typed<Typed<Damagable::Listener *> > damagablelistener("damagablelistener");
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
	for (Database::Typed<Listener *>::Iterator itor(Database::damagablelistener.Find(id)); itor.IsValid(); ++itor)
	{
		itor.GetValue()->Damage(aSourceId, aDamage);
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
