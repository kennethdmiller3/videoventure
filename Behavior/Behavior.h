#pragma once

#include "Task.h"

// forward declaration
class Controller;
class Brain;

class Behavior : public Task
{
protected:
	unsigned int mId;
	Controller *mController;

public:
	Behavior(unsigned int aId, Controller *aController)
		: mId(aId)
		, mController(aController)
	{
	}

	virtual ~Behavior()
	{
	}
};
