#pragma once

#include "Controller.h"

// player controller
class PlayerController : 
	public Controller
{
public:
	// constructor
	PlayerController(unsigned int aId = 0);

	// destructor
	~PlayerController(void);

	// control
	void Control(float aStep);
};

namespace Database
{
	extern Typed<PlayerController *> playercontroller;
}