#pragma once

#include "Entity.h"
#include "Controllable.h"
#include "Simulatable.h"
#include "Renderable.h"
#include "Input.h"
#include "Player.h"

class Gunner : 
	public Entity, public Controllable, public Simulatable, public Renderable
{
protected:
	// player
	const Player *player;

	// offset
	Matrix2 offset;

	// fire delay
	float mDelay;
	int mPhase;
	int mCycle;

public:
	// constructor
	Gunner();

	// destructor
	~Gunner(void);

	// set player
	void SetPlayer(const Player *aPlayer)
	{
		player = aPlayer;
	}

	// set phase
	void SetPhase(int aPhase)
	{
		mPhase = aPhase;
	}

	// configure
	virtual bool Configure(TiXmlElement *element);

	// control
	virtual void Control(float aStep);

	// simulate
	virtual void Simulate(float aStep);

	// render
	virtual void Render(void);
};
