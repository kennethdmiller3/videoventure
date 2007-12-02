#include "StdAfx.h"
#include "Entity.h"
#include <algorithm>

namespace Database
{
	Typed<Entity *> entity("entity");
}

Entity::List Entity::sAll;
unsigned int Entity::sNextId = 1;

Entity::Entity(unsigned int id)
: id(id)
, transform(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0))
, vel(0, 0)
{
	if (id > 0)
		entry = sAll.insert(sAll.end(), this);
}

Entity::~Entity(void)
{
	if (id > 0)
		sAll.erase(entry);
}

// configure
bool Entity::Configure(TiXmlElement *element)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x21ac415f /* "rotation" */:
		{
			float angle = 0.0f;
			if (element->QueryFloatAttribute("angle", &angle) == TIXML_SUCCESS)
				angle *= float(M_PI)/180.0f;
			transform.x.x = cosf(angle);
			transform.x.y = sinf(angle);
			transform.y.x = -transform.x.y;
			transform.y.y = transform.x.x;
		}
		return true;

	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &transform.p.x);
		element->QueryFloatAttribute("y", &transform.p.y);
		return true;

	case 0x32741c32 /* "velocity" */:
		element->QueryFloatAttribute("x", &vel.x);
		element->QueryFloatAttribute("y", &vel.y);
		return true;

	default:
		return false;
	}
}
