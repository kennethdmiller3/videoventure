#pragma once

#include "Behavior.h"

class RipOffBehavior : public Behavior
{
public:
	// target
	unsigned int mTarget;

	// attached
	bool mAttached;

	// escape dirction
	Vector2 mDirection;

public:
	RipOffBehavior(unsigned int aId, Controller *aController);
	~RipOffBehavior();

	Status Execute(void);

	// prioritized sub-behaviors
	// TO DO: build this as a selector
	Status SelectTarget(void);
	Status AvoidFriendly(void);
	Status AttackEnemy(void);
	Status SeekEscape(void);
	Status NearStealable(void);
	Status NearTarget(void);
	Status SeekTarget(void);

protected:
	unsigned int RandomTarget(void);
	unsigned int ClosestTarget(float aRadius, bool aLocked = false);

	void Escape(unsigned int aId);

	void Attach(unsigned int aTarget);
	void Detach(unsigned int aTarget);

	void Lock(unsigned int aTarget);
	void Unlock(unsigned int aTarget);
	unsigned int GetLocked(unsigned int aTarget);
};
