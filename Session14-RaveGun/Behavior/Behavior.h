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

namespace BehaviorDatabase
{
	namespace Loader
	{
		typedef fastdelegate::FastDelegate<unsigned int (unsigned int, const TiXmlElement *)> Entry;
		void AddConfigure(unsigned int aTagId, Entry aEntry);
		const Entry &GetConfigure(unsigned int aTagId);
	}

	namespace Initializer
	{
		typedef fastdelegate::FastDelegate<Behavior *(unsigned int, Controller *)> ActivateEntry;
		void AddActivate(unsigned int aDatabaseId, ActivateEntry aEntry);
		const ActivateEntry &GetActivate(unsigned int aDatabaseId);
		typedef fastdelegate::FastDelegate<void (unsigned int)> DeactivateEntry;
		void AddDeactivate(unsigned int aDatabaseId, DeactivateEntry aEntry);
		const DeactivateEntry &GetDeactivate(unsigned int aDatabaseId);
	}
}
