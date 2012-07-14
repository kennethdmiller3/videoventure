#include "StdAfx.h"

#include "RangeBehavior.h"
#include "TargetBehavior.h"

#include "Entity.h"
#include "Controller.h"


namespace Database
{
	Typed<CloseBehaviorTemplate> closebehaviortemplate(0x9b6aa00b /* "closebehaviortemplate" */);
	Typed<FarBehaviorTemplate> farbehaviortemplate(0xfa06c762 /* "farbehaviortemplate" */);
	Typed<RangeBehavior *> rangebehavior(0xc8280a04 /* "rangebehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		static unsigned int CloseBehaviorConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			CloseBehaviorTemplate &closebehavior = Database::closebehaviortemplate.Open(aId);
			closebehavior.Configure(element, aId);
			Database::closebehaviortemplate.Close(aId);
			return 0x9b6aa00b /* "closebehaviortemplate" */;
		}
		Configure closebehaviorconfigure(0x27cb3b23 /* "close" */, CloseBehaviorConfigure);

		static unsigned int FarBehaviorConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			FarBehaviorTemplate &farbehavior = Database::farbehaviortemplate.Open(aId);
			farbehavior.Configure(element, aId);
			Database::farbehaviortemplate.Close(aId);
			return 0xfa06c762 /* "farbehaviortemplate" */;
		}
		Configure farbehaviorconfigure(0xbcf819ee /* "far" */, FarBehaviorConfigure);
	}

	namespace Initializer
	{
		static Behavior *RangeBehaviorActivate(unsigned int aId, Controller *aController)
		{
			if (RangeBehavior *rangebehavior = Database::rangebehavior.Get(aId))
				return rangebehavior;
			RangeBehavior *rangebehavior = new RangeBehavior(aId, aController);
			Database::rangebehavior.Put(aId, rangebehavior);
			return rangebehavior;
		}
		Activate closebehavioractivate(0x9b6aa00b /* "closebehaviortemplate" */, RangeBehaviorActivate);
		Activate farbehavioractivate(0xfa06c762 /* "farbehaviortemplate" */, RangeBehaviorActivate);

		static void RangeBehaviorDeactivate(unsigned int aId)
		{
			if (RangeBehavior *rangebehavior = Database::rangebehavior.Get(aId))
			{
				delete rangebehavior;
				Database::rangebehavior.Delete(aId);
			}
		}
		Deactivate closebehaviordeactivate(0x9b6aa00b /* "closebehaviortemplate" */, RangeBehaviorDeactivate);
		Deactivate farbehaviordeactivate(0xfa06c762 /* "farbehaviortemplate" */, RangeBehaviorDeactivate);
	}
}


CloseBehaviorTemplate::CloseBehaviorTemplate()
: mRange(-FLT_MAX)
, mScaleDist(1.0f/16.0f)
, mScaleSpeed(0.0f)
{
}

bool CloseBehaviorTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
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

bool FarBehaviorTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
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
		return runningTask;

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

	return runningTask;
}
