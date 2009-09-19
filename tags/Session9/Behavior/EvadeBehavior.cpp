#include "StdAfx.h"

#include "EvadeBehavior.h"
#include "TargetBehavior.h"
#include "..\Entity.h"
#include "..\Controller.h"


namespace Database
{
	Typed<EvadeBehaviorTemplate> evadebehaviortemplate(0xe0f0e10a /* "evadebehaviortemplate" */);
}


EvadeBehaviorTemplate::EvadeBehaviorTemplate()
: mStrength(0.0f)
{
}

bool EvadeBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	return true;
}

EvadeBehavior::EvadeBehavior(unsigned int aId, const EvadeBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &EvadeBehavior::Execute);
}

Status EvadeBehavior::Execute(void)
{
	// get target
	const TargetData &targetdata = Database::targetdata.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(targetdata.mTarget);
	if (!targetEntity)
		return runningTask();

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// get evade behavior template
	const EvadeBehaviorTemplate &evade = Database::evadebehaviortemplate.Get(mId);

	// target entity transform
	const Transform2 &targetTransform = targetEntity->GetTransform();

	// evade target's front vector
	Vector2 local(targetTransform.Untransform(entity->GetPosition()));
	if (local.y > 0)
	{
		local *= InvSqrt(local.LengthSq());
		float dir = local.x > 0 ? 1.0f : -1.0f;
		mController->mMove += evade.mStrength * dir * local.y * local.y * local.y * targetTransform.Rotate(Vector2(local.y, -local.x));
	}

	return runningTask();
}
