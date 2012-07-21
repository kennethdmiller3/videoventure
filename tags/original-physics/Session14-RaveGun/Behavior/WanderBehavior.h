#pragma once

#include "Behavior.h"

// wander behavior
class WanderBehaviorTemplate
{
public:
	float mSide;		// side strength
	float mSideRate;	// side frequency
	float mFront;		// front strength
	float mFrontRate;	// front frequency
	float mTurn;		// turn strength
	float mTurnRate;	// turn frequency

public:
	WanderBehaviorTemplate();

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class WanderBehavior : public Behavior
{
protected:
	float mSidePhase;
	float mFrontPhase;
	float mTurnPhase;

public:
	WanderBehavior(unsigned int aId, const WanderBehaviorTemplate &aTemplate, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<WanderBehaviorTemplate> wanderbehaviortemplate;
}
