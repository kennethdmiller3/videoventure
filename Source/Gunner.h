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
	// input
	const Input *input;

	// player
	const Player *player;

	// transform;
	Vector2 axis_x;
	Vector2 axis_y;

	// fire delay
	float mDelay;
	int mPhase;
	int mCycle;

public:
	// constructor
	Gunner();

	// destructor
	~Gunner(void);

	// set input
	void SetInput(const Input *aInput)
	{
		input = aInput;
	}

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

	// control
	void Control(float aStep);

	// simulate
	void Simulate(float aStep);

	// render
	void Render(void);
};
