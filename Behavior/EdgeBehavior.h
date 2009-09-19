#pragma once

#include "Behavior.h"

// edge behavior
class EdgeBehaviorTemplate
{
public:
	float mStrength;
	float mDistance;

public:
	EdgeBehaviorTemplate();

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class EdgeBehavior : public Behavior
{
public:
	EdgeBehavior(unsigned int aId, const EdgeBehaviorTemplate &aTemplate, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<EdgeBehaviorTemplate> edgebehaviortemplate;
}
