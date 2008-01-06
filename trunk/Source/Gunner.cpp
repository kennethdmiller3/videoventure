#include "StdAfx.h"
#include "Gunner.h"
#include "Entity.h"

namespace Database
{
	Typed<Gunner *> gunner("gunner");
}

// Gunner Constructor
Gunner::Gunner(unsigned int aId)
: Simulatable(aId)
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
			{
				unsigned int owner = Hash(child->Attribute("name"));
				Database::owner.Put(id, Database::owner.Get(owner));
			}
			break;
		}
	}

	return true;
}

// Gunner Simulate
void Gunner::Simulate(float aStep)
{
	// get the owner
	unsigned int aOwnerId = Database::owner.Get(id);

	// if the owner does not exist...
	if (!Database::entity.Find(aOwnerId))
	{
		// self-destruct
		Database::Delete(id);
		return;
	}
}
