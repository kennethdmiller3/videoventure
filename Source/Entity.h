#pragma once

#include <list>

class Entity
{
private:
	// next available identifier
	static unsigned int sNextId;

protected:
	// identifier
	unsigned int id;

	// previous and current transform
	Transform2 trans[2];

	// current velocity
	Transform2 veloc;

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
		trans[0] = trans[1];
	}

	// set transform
	void SetTransform(const float aAngle, const Vector2 &aPosit)
	{
		trans[1].a = aAngle;
		trans[1].p = aPosit;
	}
	void SetTransform(const Transform2 &aTransform)
	{
		trans[1] = aTransform;
	}

	// get transform
	const Transform2 GetTransform() const
	{
		return trans[1];
	}

	// get interpolated transform
	const Transform2 GetInterpolatedTransform(float aRatio) const
	{
		return Transform2(GetInterpolatedAngle(aRatio), GetInterpolatedPosition(aRatio));
	}

	// set previous angle
	void SetPrevAngle(float aAngle)
	{
		trans[0].a = aAngle;
	}

	// set angle
	void SetAngle(float aAngle)
	{
		trans[1].a = aAngle;
	}

	// get previous angle
	float GetPrevAngle() const
	{
		return trans[0].a;
	}

	// get current angle
	float GetAngle() const
	{
		return trans[1].a;
	}

	// get interpolated angle
	float GetInterpolatedAngle(float aRatio) const
	{
		float angle_d = trans[1].a - trans[0].a;
		if (angle_d > float(M_PI))
			angle_d -= 2.0f*float(M_PI);
		else if (angle_d < -float(M_PI))
			angle_d += 2.0f*float(M_PI);
		return trans[0].a + angle_d * aRatio;
	}

	// set veloc.a
	void SetOmega(float aOmega)
	{
		veloc.a = aOmega;
	}

	// get veloc.a
	const float GetOmega() const
	{
		return veloc.a;
	}

	// set previous position
	void SetPrevPosition(const Vector2 &aPos)
	{
		trans[0].p = aPos;
	}

	// set position
	void SetPosition(const Vector2 &aPos)
	{
		trans[1].p = aPos;
	}

	// get previous position
	const Vector2 &GetPrevPosition() const
	{
		return trans[0].p;
	}

	// get current position
	const Vector2 &GetPosition() const
	{
		return trans[1].p;
	}

	// get interpolated position
	const Vector2 GetInterpolatedPosition(float aRatio) const
	{
		return trans[0].p + (trans[1].p - trans[0].p) * aRatio;
	}


	// set velocity
	void SetVelocity(const Vector2 &aVel)
	{
		veloc.p = aVel;
	}

	// get velocity
	const Vector2 &GetVelocity() const
	{
		return veloc.p;
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
