#pragma once

#include "Behavior.h"

class Aimer;

// target behavior
class TargetBehaviorTemplate
{
public:
	float mPeriod;		// time between scans
	float mRange;		// maximum range
	float mDirection;	// direction angle
	float mAngle;		// cone angle
	float mFocus;		// weight factor for current target
	float mAlign;		// weight factor for angle alignment
	b2Filter mFilter;	// collision filtering

public:
	TargetBehaviorTemplate();

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

struct TargetData
{
	unsigned int mTarget;
	Vector2 mOffset;
};

class TargetBehavior : public Behavior
{
public:
	float mDelay;

public:
	TargetBehavior(unsigned int aId, const TargetBehaviorTemplate &aTemplate, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<TargetBehaviorTemplate> targetbehaviortemplate;
	extern Typed<TargetData> targetdata;
}
