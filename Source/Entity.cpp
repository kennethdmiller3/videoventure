#include "StdAfx.h"
#include "Entity.h"
#include <algorithm>

Entity::List Entity::sAll;
int Entity::sNextId = 1;

Entity::Entity(void)
: id(sNextId++)
, pos(0, 0)
, vel(0, 0)
{
	entry = sAll.insert(sAll.end(), this);
}

Entity::~Entity(void)
{
	sAll.erase(entry);
}
