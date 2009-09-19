#pragma once

#include "Behavior.h"

// evade behavior
class EvadeBehaviorTemplate
{
public:
	float mStrength;	// evasion strength

public:
	EvadeBehaviorTemplate();

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class EvadeBehavior : public Behavior
{
public:
	EvadeBehavior(unsigned int aId, const EvadeBehaviorTemplate &aTemplate, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<EvadeBehaviorTemplate> evadebehaviortemplate;
}
