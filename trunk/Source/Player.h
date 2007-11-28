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

	// transform
	Vector2 axis_x;
	Vector2 axis_y;

	// fire delay
	float mDelay;
	int mCycle;

public:
	// constructor
	Player(void);

	// destructor
	~Player(void);

	// set input
	void SetInput(const Input *aInput)
	{
		input = aInput;
	}

	// get x axis
	const Vector2 &GetAxisX() const
	{
		return axis_x;
	}

	// get y axis
	const Vector2 &GetAxisY() const
	{
		return axis_y;
	};

	// control
	void Control(float aStep);

	// simulate
	void Simulate(float aStep);

	// collide
	void Collide(float aStep, Collidable &aRecipient);

	// render
	void Render(void);
};
