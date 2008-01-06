#pragma once

#include "Entity.h"
#include "Controllable.h"
#include "Simulatable.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Input.h"

// player actor
class Player : 
	public Controllable, public Simulatable
{
public:
	// physics parameters
	float mMaxVeloc;
	float mMaxAccel;
	float mFriction;
	float mMaxOmega;

	// control values
	Vector2 mMove;
	Vector2 mAim;

public:
	// constructor
	Player(unsigned int aId = 0, unsigned int aParentId = 0);

	// destructor
	~Player(void);

	// configure
	virtual bool Configure(TiXmlElement *element);

	// control
	virtual void Control(float aStep);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<Player *> player;
}