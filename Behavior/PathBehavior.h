#pragma once

#include "Behavior.h"

// path behavior
class PathBehaviorTemplate
{
public:
	float mStrength;	// path strength
	std::vector<unsigned int> mAction;

public:
	PathBehaviorTemplate();

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};


class PathBehavior : public Behavior
{
public:
	PathBehavior(unsigned int aId, const PathBehaviorTemplate &aTemplate, Controller *aController);

	Status Execute(void);
};

namespace Database
{
	extern Typed<PathBehaviorTemplate> pathbehaviortemplate;
}
