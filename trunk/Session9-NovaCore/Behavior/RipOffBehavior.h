#pragma once

#include "Behavior.h"
#include "Collidable.h"

class Entity;

class RipOffBehaviorTemplate
{
public:
	unsigned int mTargetTag;

	CollidableFilter mAvoidFilter;
	float mAvoidRadius;

	CollidableFilter mAttackFilter;
	float mAttackRadius;

	float mCloseRadius;
	float mCloseScaleDist;
	float mCloseScaleSpeed;

public:
	RipOffBehaviorTemplate(void);

	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

class RipOffBehavior : public Behavior
{
public:
	// parent entity
	Entity *mEntity;

	// target
	unsigned int mTarget;

	// escape dirction
	Vector2 mDirection;

public:
	RipOffBehavior(unsigned int aId, Controller *aController);
	~RipOffBehavior();

	// sub-behaviors
	// TO DO: build this as a behavior tree
	Status PickRandomTarget(void);
	Status PickClosestTarget(void);
	Status AvoidFriendly(void);
	Status AttackEnemy(void);
	Status FlyToTarget(void);
	Status TurnAround(void);
	Status AttachToTarget(void);
	Status PickRandomDirection(void);
	Status FlyInDirection(void);

protected:
	unsigned int RandomTarget(void);
	unsigned int ClosestTarget(float aRadius, bool aLocked);
	unsigned int ClosestEntity(float aRadius, const CollidableFilter &aFilter);

	void Drive(float aSpeed, Vector2 aDirection);

	void Escape(unsigned int aId1, unsigned int aId2, float aTime, const Vector2 &aContact, const Vector2 &aNormal);

	void Attach(unsigned int aTarget);
	void Detach(unsigned int aTarget);

	void Lock(unsigned int aTarget);
	void Unlock(unsigned int aTarget);
	unsigned int GetLocked(unsigned int aTarget);
};

namespace Database
{
	extern Typed<RipOffBehaviorTemplate> ripoffbehaviortemplate;
}
