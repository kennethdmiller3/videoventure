#pragma once

#include <list>

class Entity
{
private:
	// list of all entities
	typedef std::list<Entity *> List;
	static List sAll;

	// next available identifier
	static unsigned int sNextId;

	// list entry
	List::iterator entry;

protected:
	// identifier
	unsigned int id;

	// previous transform
	float angle_0;
	Vector2 posit_0;

	// current transform
	float angle_1;
	Vector2 posit_1;

	// current velocity
	Vector2 vel;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// constructor
	Entity(unsigned int id = 0);

	// destructor
	virtual ~Entity(void);

	// take next identifier
	static unsigned int TakeId(void)
	{
		return sNextId++;
	}

	// get identifier
	unsigned int GetId(void) const
	{
		return id;
	}

	// step
	void Step(void)
	{
		angle_0 = angle_1;
		posit_0 = posit_1;
	}

	// set transform
	void SetTransform(const float aAngle, const Vector2 &aPosit)
	{
		angle_1 = aAngle;
		posit_1 = aPosit;
	}
	void SetTransform(const Matrix2 &aTransform)
	{
		SetTransform(aTransform.Angle(), aTransform.p);
	}

	// get transform
	const Matrix2 GetTransform() const
	{
		return Matrix2(angle_1, posit_1);
	}

	// get interpolated transform
	const Matrix2 GetInterpolatedTransform(float aRatio) const
	{
		return Matrix2(GetInterpolatedAngle(aRatio), GetInterpolatedPosition(aRatio));
	}

	// st angle
	void SetAngle(float aAngle)
	{
		angle_1 = aAngle;
	}

	// get previous angle
	float GetPrevAngle() const
	{
		return angle_0;
	}

	// get current angle
	float GetAngle() const
	{
		return angle_1;
	}

	// get interpolated angle
	float GetInterpolatedAngle(float aRatio) const
	{
		float angle_d = angle_1 - angle_0;
		if (angle_d > float(M_PI))
			angle_d -= 2.0f*float(M_PI);
		else if (angle_d < -float(M_PI))
			angle_d += 2.0f*float(M_PI);
		return angle_0 + angle_d * aRatio;
	}

	// set position
	void SetPosition(const Vector2 &aPos)
	{
		posit_1 = aPos;
	}

	// get previous position
	const Vector2 &GetPrevPosition() const
	{
		return posit_0;
	}

	// get current position
	const Vector2 &GetPosition() const
	{
		return posit_1;
	}

	// get interpolated position
	const Vector2 GetInterpolatedPosition(float aRatio) const
	{
		return posit_0 + (posit_1 - posit_0) * aRatio;
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
	virtual bool Configure(const TiXmlElement *element);

	// init
	virtual void Init(void)
	{
		Step();
	}
};

namespace Database
{
	extern Typed<Entity *> entity;
}
