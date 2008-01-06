#include "StdAfx.h"
#include "Gunner.h"
#include "Bullet.h"

namespace Database
{
	Typed<Gunner *> gunner("gunner");
}

// Gunner Constructor
Gunner::Gunner(unsigned int aId, unsigned int aParentId)
: Simulatable(aId)
, owner(0)
{
}

// Gunner Destructor
Gunner::~Gunner(void)
{
}

// configure
bool Gunner::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0xe063cbaa /* "gunner" */)
		return false;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xf5674cd4 /* "owner" */:
			owner = Hash(child->Attribute("name"));
			break;
		}
	}

	return true;
}

// Gunner Simulate
void Gunner::Simulate(float aStep)
{
	if (!owner)
		return;

	// if the owner does not exist...
	if (!Database::entity.Get(owner))
	{
		// self-destruct
		Database::Delete(Simulatable::id);
		return;
	}
}
