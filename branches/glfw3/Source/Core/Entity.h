#pragma once

class GAME_API Entity
{
private:
	// next available identifier
	static unsigned int sNextId;

protected:
	// identifier
	unsigned int id;

	// previous and current transform
	Transform2 prevtrans, curtrans;

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
		prevtrans = curtrans;
	}

	// set transform
	void SetTransform(const float aAngle, const Vector2 &aPosit)
	{
		curtrans.a = aAngle;
		curtrans.p = aPosit;
	}
	void SetTransform(const Transform2 &aTransform)
	{
		curtrans = aTransform;
	}

	// get transform
	const Transform2 &GetTransform() const
	{
		return curtrans;
	}

	// get interpolated transform
	const Transform2 GetInterpolatedTransform(float aRatio) const
	{
		return Transform2(GetInterpolatedAngle(aRatio), GetInterpolatedPosition(aRatio));
	}

	// set previous angle
	void SetPrevAngle(float aAngle)
	{
		prevtrans.a = aAngle;
	}

	// set angle
	void SetAngle(float aAngle)
	{
		curtrans.a = aAngle;
	}

	// get previous angle
	float GetPrevAngle() const
	{
		return prevtrans.a;
	}

	// get current angle
	float GetAngle() const
	{
		return curtrans.a;
	}

	// get interpolated angle
	float GetInterpolatedAngle(float aRatio) const
	{
		float angle_d = curtrans.a - prevtrans.a;
		if (angle_d > float(M_PI))
			angle_d -= 2.0f*float(M_PI);
		else if (angle_d < -float(M_PI))
			angle_d += 2.0f*float(M_PI);
		return prevtrans.a + angle_d * aRatio;
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
		prevtrans.p = aPos;
	}

	// set position
	void SetPosition(const Vector2 &aPos)
	{
		curtrans.p = aPos;
	}

	// get previous position
	const Vector2 &GetPrevPosition() const
	{
		return prevtrans.p;
	}

	// get current position
	const Vector2 &GetPosition() const
	{
		return curtrans.p;
	}

	// get interpolated position
	const Vector2 GetInterpolatedPosition(float aRatio) const
	{
		return prevtrans.p + (curtrans.p - prevtrans.p) * aRatio;
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
	virtual bool Configure(const tinyxml2::XMLElement *element);

	// init
	virtual void Init(void)
	{
		Step();
	}
};

namespace Database
{
	extern GAME_API Typed<Entity *> entity;
}
