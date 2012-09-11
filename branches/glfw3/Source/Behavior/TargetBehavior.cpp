#include "StdAfx.h"

#include "TargetBehavior.h"
#include "Collidable.h"
#include "Entity.h"
#include "Team.h"
#include "Damagable.h"
#include "Cancelable.h"
#include "Aimer.h"

namespace Database
{
	Typed<TargetBehaviorTemplate> targetbehaviortemplate(0x5dfd8444 /* "targetbehaviortemplate" */);
	Typed<TargetData> targetdata(0xcaaa7b50 /* "targetdata" */);
	Typed<TargetBehavior *> targetbehavior(0xaaed8862 /* "targetbehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		static unsigned int TargetBehaviorConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			TargetBehaviorTemplate &target = Database::targetbehaviortemplate.Open(aId);
			target.Configure(element, aId);
			Database::targetbehaviortemplate.Close(aId);
			return 0x5dfd8444 /* "targetbehaviortemplate" */;
		}
		Configure targetbehaviorconfigure(0x32608848 /* "target" */, TargetBehaviorConfigure);
	}

	namespace Initializer
	{
		static Behavior *TargetBehaviorActivate(unsigned int aId, Controller *aController)
		{
			const TargetBehaviorTemplate &targetbehaviortemplate = Database::targetbehaviortemplate.Get(aId);
			TargetBehavior *targetbehavior = new TargetBehavior(aId, targetbehaviortemplate, aController);
			Database::targetbehavior.Put(aId, targetbehavior);
			return targetbehavior;
		}
		Activate targetbehavioractivate(0x5dfd8444 /* "targetbehaviortemplate" */, TargetBehaviorActivate);

		static void TargetBehaviorDeactivate(unsigned int aId)
		{
			if (TargetBehavior *targetbehavior = Database::targetbehavior.Get(aId))
			{
				delete targetbehavior;
				Database::targetbehavior.Delete(aId);
			}
		}
		Deactivate targetbehaviordeactivate(0x5dfd8444 /* "targetbehaviortemplate" */, TargetBehaviorDeactivate);
	}
}

TargetBehaviorTemplate::TargetBehaviorTemplate()
: mPeriod(1.0f)
, mRange(0.0f)
, mDirection(0.0f)
, mAngle(float(M_PI)*2.0f)
, mFocus(1.0f)
, mAlign(0.0f)
, mFilter(Collidable::GetDefaultFilter())
{
}

bool TargetBehaviorTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("period", &mPeriod);
	element->QueryFloatAttribute("range", &mRange);
	element->QueryFloatAttribute("direction", &mDirection);
	element->QueryFloatAttribute("angle", &mAngle);
	element->QueryFloatAttribute("focus", &mFocus);
	element->QueryFloatAttribute("align", &mAlign);
	ConfigureFilterData(mFilter, element);
	return true;
}

TargetBehavior::TargetBehavior(unsigned int aId, const TargetBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
, mDelay(aTemplate.mPeriod * aId / UINT_MAX)
{
	bind(this, &TargetBehavior::Execute);

	// create a target data
	// TO DO: remove target data when done
	Database::targetdata.Open(aId);
	Database::targetdata.Close(aId);
}

class TargetQueryCallback
{
public:
	TargetBehaviorTemplate mTarget;
	unsigned int mId;
	unsigned int mTeam;
	Transform2 mTransform;
	unsigned int mCurrentTarget;

	float mBestRange;
	unsigned int mBestTargetId;
	Vector2 mBestTargetPos;

public:
	TargetQueryCallback(const TargetBehaviorTemplate &aTarget, unsigned int aId, unsigned int aTeam, Transform2 aTransform, unsigned int aCurrentTarget)
		: mTarget(aTarget)
		, mId(aId)
		, mTeam(aTeam)
		, mTransform(aTransform)
		, mCurrentTarget(aCurrentTarget)
		, mBestRange(FLT_MAX)
		, mBestTargetId(0)
		, mBestTargetPos(0, 0)
	{
	}

	void Report(CollidableShape *shape, float range, const Vector2 &point)
	{
		// skip unhittable fixtures
		if (Collidable::IsSensor(shape))
			return;
		if (!Collidable::CheckFilter(mTarget.mFilter, Collidable::GetFilter(shape)))
			return;

		// get the collidable identifier
		unsigned int targetId = Collidable::GetId(shape);

		// skip non-entity
		if (targetId == 0)
			return;

		// skip self
		if (targetId == mId)
			return;

		// get team affiliation
		unsigned int targetTeam = Database::team.Get(targetId);

		// skip neutral
		if (targetTeam == 0)
			return;

		// skip teammate
		if (targetTeam == mTeam)
			return;

		// skip indestructible
		if (!Database::damagable.Find(targetId) && !Database::cancelable.Find(targetId))
			return;

		// get center position
		Vector2 centerPos(Collidable::GetCenter(shape));

		// get local direction
		Vector2 localDir(mTransform.Untransform(centerPos));

		// if using a cone angle or angle scale
		if (mTarget.mAngle < float(M_PI)*2.0f || mTarget.mAlign != 0.0f)
		{
			// get angle to target
			float aimAngle = -atan2f(localDir.x, localDir.y);

			// get local angle
			float localAngle = aimAngle - mTarget.mDirection;
			if (localAngle > float(M_PI))
				localAngle -= float(M_PI)*2.0f;
			else if (localAngle < -float(M_PI))
				localAngle += float(M_PI)*2.0f;

			// skip if outside angle
			if (fabsf(localAngle) > mTarget.mAngle)
				return;

			// if using angle scale...
			if (mTarget.mAlign != 0.0f)
			{
				// apply angle scale
				range *= 1.0f + fabsf(localAngle) * mTarget.mAlign;
			}
		}

		// if the current target...
		if (targetId == mCurrentTarget)
		{
			// apply focus scale
			range /= mTarget.mFocus;
		}

		// if better than the current range
		if (mBestRange > range)
		{
			// use the new target
			mBestRange = range;
			mBestTargetId = targetId;
			mBestTargetPos = Database::entity.Get(targetId)->GetTransform().Untransform(centerPos);
		}
	}
};

// target behavior
Status TargetBehavior::Execute(void)
{
	const TargetBehaviorTemplate &target = Database::targetbehaviortemplate.Get(mId);

	// if ready to search...
	mDelay -= sim_step;
	if (mDelay > 0.0f)
		return runningTask;

	// update the timer
	mDelay += target.mPeriod;

	// get the owner entity
	Entity *entity = Database::entity.Get(mId);
	if (!entity)
		return runningTask;

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get team affiliation
	unsigned int aTeam = Database::team.Get(mId);

	// current target properties
	TargetData &data = Database::targetdata.Open(mId);
	unsigned int &mTarget = data.mTarget;
	Vector2 &mOffset = data.mOffset;

	// query nearby fixtures
	TargetQueryCallback callback(target, mId, aTeam, transform, data.mTarget);
	Collidable::QueryRadius(entity->GetPosition(), target.mRange, target.mFilter,
		Collidable::QueryRadiusDelegate(&callback, &TargetQueryCallback::Report));

	// use the new target
	mTarget = callback.mBestTargetId;
	mOffset = callback.mBestTargetPos;
	return runningTask;
}

