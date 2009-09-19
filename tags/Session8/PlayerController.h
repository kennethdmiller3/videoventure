#pragma once

#include "Controller.h"

// player controller template
class PlayerControllerTemplate
{
public:
	enum ControlType
	{
		NONE,
		MOVESTEER,
		MOVELOCAL,
		MOVEWORLD,
		MOVECURSOR,
		AIMSTEER,
		AIMLOCAL,
		AIMWORLD,
		AIMCURSOR,
		LEFT,
		RIGHT,
		UP,
		DOWN,
		NUM_TYPES
	};
	ControlType mMove;
	ControlType mAim;
	Transform2 mScale;
	Transform2 mAdd;

public:
	PlayerControllerTemplate()
		: mMove(MOVEWORLD)
		, mAim(AIMWORLD)
		, mScale(1, Vector2(1, 1))
		, mAdd(0, Vector2(0, 0))
	{
	}
};

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
	extern Typed<PlayerControllerTemplate> playercontrollertemplate;
	extern Typed<PlayerController *> playercontroller;
}