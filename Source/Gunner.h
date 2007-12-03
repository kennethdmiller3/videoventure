#pragma once

#include "Entity.h"
#include "Controllable.h"
#include "Simulatable.h"
#include "Renderable.h"
#include "Input.h"
#include "Player.h"

class Gunner : 
	public Entity, public Controllable, public Simulatable, public Collidable, public Renderable
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
	Gunner(unsigned int aId = 0, unsigned int aParentId = 0);

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

	// init
	virtual void Init(void);

	// control
	virtual void Control(float aStep);

	// simulate
	virtual void Simulate(float aStep);

	// render
	virtual void Render(const Matrix2 &transform);
};
