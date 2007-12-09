#pragma once

#include "Entity.h"
#include "Controllable.h"
#include "Simulatable.h"
#include "Renderable.h"
#include "Input.h"
#include "Player.h"

// gunner actor
class Gunner : 
	public Controllable, public Simulatable
{
protected:
	// owner
	unsigned int owner;

	// offset
	Matrix2 offset;

	// control values
	bool mFire;

	// fire delay
	float mDelay;
	int mPhase;
	int mCycle;

public:
	// constructor
	Gunner(unsigned int aId = 0, unsigned int aParentId = 0);

	// destructor
	~Gunner(void);

	// set phase
	void SetPhase(int aPhase)
	{
		mPhase = aPhase;
	}

	// configure
	virtual bool Configure(TiXmlElement *element);

	// init
	virtual void Init(void);

	// control
	virtual void Control(float aStep);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<Gunner *> gunner;
}