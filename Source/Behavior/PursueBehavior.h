#pragma once

#include "Behavior.h"

// pursue behavior
class PursueBehaviorTemplate
{
public:
	float mStrength;	// pursuit strength
	float mLeading;		// leading speed
	Transform2 mOffset;	// offset transform

public:
	PursueBehaviorTemplate();

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};


class PursueBehavior : public Behavior
{
public:
	PursueBehavior(unsigned int aId, const PursueBehaviorTemplate &aTemplate, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<PursueBehaviorTemplate> pursuebehaviortemplate;
}
