#pragma once

#include "Scheduler.h"

class Brain
{
public:
	Brain(void)
	{
	}

	void Think(float aStep)
	{
		mScheduler.Update();
	}

protected:
	Scheduler mScheduler;
};
