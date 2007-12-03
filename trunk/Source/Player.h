#pragma once

#include "Entity.h"
#include "Controllable.h"
#include "Simulatable.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Input.h"

class Player : 
	public Entity, public Controllable, public Simulatable, public Collidable, public Renderable
{
	// input
	const Input *input;

public:
	// physics parameters
	float mMaxVeloc;
	float mMaxAccel;
	float mFriction;
	float mMaxOmega;

	// fire delay
	float mDelay;
	int mCycle;

public:
	// constructor
	Player(unsigned int aId = 0, unsigned int aParentId = 0);

	// destructor
	~Player(void);

	// get input
	const Input *GetInput(void) const
	{
		return input;
	}

	// set input
	void SetInput(const Input *aInput)
	{
		input = aInput;
	}

	// configure
	virtual bool Configure(TiXmlElement *element);

	// control
	virtual void Control(float aStep);

	// simulate
	virtual void Simulate(float aStep);

	// collide
	virtual void Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount);

	// render
	virtual void Render(const Matrix2 &transform);
};
