#pragma once

#include "Controller.h"

// player controller
class Player : 
	public Controller
{
public:
	// constructor
	Player(unsigned int aId = 0);

	// destructor
	~Player(void);

	// configure
	virtual bool Configure(const TiXmlElement *element);

	// control
	virtual void Control(float aStep);
};

namespace Database
{
	extern Typed<Player *> player;
}