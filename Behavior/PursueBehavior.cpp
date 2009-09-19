#include "StdAfx.h"

#include "PursueBehavior.h"
#include "TargetBehavior.h"
#include "BotUtilities.h"
#include "..\Controller.h"
#include "..\Entity.h"

namespace Database
{
	Typed<PursueBehaviorTemplate> pursuebehaviortemplate(0xb9b0800f /* "pursuebehaviortemplate" */);
}

PursueBehaviorTemplate::PursueBehaviorTemplate()
: mStrength(0.0f)
, mLeading(0.0f)
{
}

bool PursueBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	element->QueryFloatAttribute("leading", &mLeading);
	return true;
}

PursueBehavior::PursueBehavior(unsigned int aId, const PursueBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &PursueBehavior::Execute);
}

// pursue behavior
Status PursueBehavior::Execute(void)
{
	// get target
	const TargetData &targetdata = Database::targetdata.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(targetdata.mTarget);
	if (!targetEntity)
		return runningTask();

	// get pursue behavior template
	const PursueBehaviorTemplate &pursue = Database::pursuebehaviortemplate.Get(mId);

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// direction to target
	Vector2 targetDir(TargetDir(pursue.mLeading, entity, targetEntity, targetdata.mOffset));

	// save range
	float distSq = targetDir.LengthSq();

	// normalize direction
	targetDir *= InvSqrt(distSq);

	// move towards target
	mController->mMove += pursue.mStrength * targetDir;

	return runningTask();
}
