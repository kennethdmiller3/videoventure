#include "StdAfx.h"

#include "RangeBehavior.h"
#include "TargetBehavior.h"

#include "..\Entity.h"
#include "..\Controller.h"


namespace Database
{
	Typed<CloseBehaviorTemplate> closebehaviortemplate(0x9b6aa00b /* "closebehaviortemplate" */);
	Typed<FarBehaviorTemplate> farbehaviortemplate(0xfa06c762 /* "farbehaviortemplate" */);
}


CloseBehaviorTemplate::CloseBehaviorTemplate()
: mRange(-FLT_MAX)
, mScaleDist(1.0f/16.0f)
, mScaleSpeed(0.0f)
{
}

bool CloseBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("range", &mRange);
	element->QueryFloatAttribute("scaledist", &mScaleDist);
	element->QueryFloatAttribute("scalespeed", &mScaleSpeed);
	return true;
}

FarBehaviorTemplate::FarBehaviorTemplate()
: mRange(FLT_MAX)
, mScaleDist(1.0f/64.0f)
, mScaleSpeed(0.0f)
{
}

bool FarBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("range", &mRange);
	element->QueryFloatAttribute("scaledist", &mScaleDist);
	element->QueryFloatAttribute("scalespeed", &mScaleSpeed);
	return true;
}


RangeBehavior::RangeBehavior(unsigned int aId, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &RangeBehavior::Execute);
}

Status RangeBehavior::Execute(void)
{
	// get target
	const TargetData &targetdata = Database::targetdata.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(targetdata.mTarget);
	if (!targetEntity)
		return runningTask();

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// get range behavior templates
	const CloseBehaviorTemplate &closebehavior = Database::closebehaviortemplate.Get(mId);
	const FarBehaviorTemplate &farbehavior = Database::farbehaviortemplate.Get(mId);

	// get direction and distance to target
	Vector2 dir = targetEntity->GetPosition() - entity->GetPosition();
	float dist = dir.Length();
	dir /= dist;

	// get target relative speed
	Vector2 vel = targetEntity->GetVelocity() - entity->GetVelocity();
	float speed = vel.Dot(dir);

	// apply close-repel force
	float repel = (dist - closebehavior.mRange) * closebehavior.mScaleDist + speed * closebehavior.mScaleSpeed;
	if (repel < 0.0f)
	{
		mController->mMove += dir * repel;
	}

	// apply far-attract force
	float attract = (dist - farbehavior.mRange) * farbehavior.mScaleDist + speed * farbehavior.mScaleSpeed;
	if (attract > 0.0f)
	{
		mController->mMove += dir * attract;
	}

	return runningTask();
}
