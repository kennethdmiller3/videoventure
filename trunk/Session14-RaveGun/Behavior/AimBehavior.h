#pragma once

#include "Behavior.h"

// fire cone
class FireConeTemplate
{
public:
	int mChannel;		// fire channel
	float mRange;		// maximum range
	float mDirection;	// direction angle
	float mAngle;		// cone angle

public:
	FireConeTemplate();

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

// aim behavior
class AimBehaviorTemplate
{
public:
	float mStrength;	// pursuit strength
	float mLeading;		// leading speed

public:
	AimBehaviorTemplate();

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class AimBehavior : public Behavior
{
public:
	AimBehavior(unsigned int aId, const AimBehaviorTemplate &aTemplate, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<AimBehaviorTemplate> aimbehaviortemplate;
	extern Typed<Typed<FireConeTemplate> > fireconetemplate;
}
