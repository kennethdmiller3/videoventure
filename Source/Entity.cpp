#include "StdAfx.h"
#include "Entity.h"
#include <algorithm>

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// entity pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Entity));
void *Entity::operator new(size_t aSize)
{
	return pool.malloc();
}
void Entity::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<Entity *> entity(0xd33ff5da /* "entity" */);
}


unsigned int Entity::sNextId = 1;

Entity::Entity(unsigned int id)
: id(id)
, angle_0(0), posit_0(0, 0)
, angle_1(0), posit_1(0, 0)
, veloc(0, 0)
, omega(0)
{
}

Entity::~Entity(void)
{
}

// configure
bool Entity::Configure(const TiXmlElement *element)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &posit_1.x);
		element->QueryFloatAttribute("y", &posit_1.y);
		if (element->QueryFloatAttribute("angle", &angle_1) == TIXML_SUCCESS)
			angle_1 *= float(M_PI) / 180.0f;
		return true;

	case 0x32741c32 /* "velocity" */:
		element->QueryFloatAttribute("x", &veloc.x);
		element->QueryFloatAttribute("y", &veloc.y);
		if (element->QueryFloatAttribute("angle", &omega) == TIXML_SUCCESS)
			omega *= float(M_PI) / 180.0f;
		return true;

	case 0xf5674cd4 /* "owner" */:
		Database::owner.Put(id, Hash(element->Attribute("name")));
		return true;

	default:
		return false;
	}
}
