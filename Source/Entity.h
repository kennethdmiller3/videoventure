#pragma once

#include <list>

class Entity
{
private:
	// list of all entities
	typedef std::list<Entity *> List;
	static List sAll;

	// next available identifier
	static int sNextId;

	// list entry
	List::iterator entry;

protected:
	// identifier
	int id;

	// current position
	Vector2 pos;

	// current velocity
	Vector2 vel;

public:
	// constructor
	Entity(void);

	// destructor
	virtual ~Entity(void);

	// get identifier
	int GetId(void)
	{
		return id;
	}

	// set position
	void SetPosition(const Vector2 &aPos)
	{
		pos = aPos;
	}

	// get position
	const Vector2 &GetPosition() const
	{
		return pos;
	}

	// set velocity
	void SetVelocity(const Vector2 &aVel)
	{
		vel = aVel;
	}

	// get velocity
	const Vector2 &GetVelocity() const
	{
		return vel;
	}
};
