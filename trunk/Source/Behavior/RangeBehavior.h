#pragma once

#include "Behavior.h"

// close behavior
class CloseBehaviorTemplate
{
public:
	float mRange;		// target distance
	float mScaleDist;	// proportional gain
	float mScaleSpeed;	// derivative gain

public:
	CloseBehaviorTemplate();

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

// far behavior
class FarBehaviorTemplate
{
public:
	float mRange;		// target distance
	float mScaleDist;	// proportional gain
	float mScaleSpeed;	// derivative gain

public:
	FarBehaviorTemplate();

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

class RangeBehavior : public Behavior
{
public:
	RangeBehavior(unsigned int aId, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<CloseBehaviorTemplate> closebehaviortemplate;
	extern Typed<FarBehaviorTemplate> farbehaviortemplate;
}
