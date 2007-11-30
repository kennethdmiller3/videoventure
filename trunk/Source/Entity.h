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

	// current transform
	Matrix2 transform;

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

	// set transform
	void SetTransform(const Matrix2 &aTransform)
	{
		transform = aTransform;
	}

	// get transform
	const Matrix2 &GetTransform() const
	{
		return transform;
	}

	// set position
	void SetPosition(const Vector2 &aPos)
	{
		transform.p = aPos;
	}

	// get x axis
	const Vector2 &GetAxisX() const
	{
		return transform.x;
	}

	// get y axis
	const Vector2 &GetAxisY() const
	{
		return transform.y;
	};

	// get position
	const Vector2 &GetPosition() const
	{
		return transform.p;
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

	// configure
	virtual bool Configure(TiXmlElement *element);
};
