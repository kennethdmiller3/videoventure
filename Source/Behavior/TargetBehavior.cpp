#include "StdAfx.h"

#include "TargetBehavior.h"
#include "..\Collidable.h"
#include "..\Entity.h"
#include "..\Team.h"
#include "..\Damagable.h"
#include "..\Aimer.h"

namespace Database
{
	Typed<TargetBehaviorTemplate> targetbehaviortemplate(0x5dfd8444 /* "targetbehaviortemplate" */);
	Typed<TargetData> targetdata(0xcaaa7b50 /* "targetdata" */);
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

bool TargetBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
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

TargetBehavior::TargetBehavior(unsigned int aId, const TargetBehaviorTemplate &aTemplate, Aimer *aController)
: Behavior(aId, aController)
, mDelay(aTemplate.mPeriod * aId / UINT_MAX)
{
	bind(this, &TargetBehavior::Execute);

	// create a target data
	// TO DO: remove target data when done
	Database::targetdata.Open(aId);
	Database::targetdata.Close(aId);
}

// target behavior
Status TargetBehavior::Execute(void)
{
	const TargetBehaviorTemplate &target = Database::targetbehaviortemplate.Get(mId);

	// get the owner entity
	Entity *entity = Database::entity.Get(mId);
	if (!entity)
		return runningTask;

	// if ready to search...
	mDelay -= sim_step;
	if (mDelay > 0.0f)
		return runningTask;

	// update the timer
	mDelay += target.mPeriod;

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get the collision world
	b2World *world = Collidable::GetWorld();

	// get nearby shapes
	b2AABB aabb;
	const float lookRadius = target.mRange;
	aabb.lowerBound.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
	aabb.upperBound.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
	b2Shape* shapes[b2_maxProxies];
	int32 count = world->Query(aabb, shapes, b2_maxProxies);

	// get team affiliation
	unsigned int aTeam = Database::team.Get(mId);

	// current target properties
	TargetData &data = Database::targetdata.Open(mId);
	unsigned int &mTarget = data.mTarget;
	Vector2 &mOffset = data.mOffset;

	// no target yet
	unsigned int bestTargetId = 0;
	Vector2 bestTargetPos(0, 0);
	float bestRange = FLT_MAX;

	// for each shape...
	for (int32 i = 0; i < count; ++i)
	{
		// skip unhittable shapes
		if (shapes[i]->IsSensor())
			continue;
		if (!Collidable::CheckFilter(shapes[i]->GetFilterData(), target.mFilter))
			continue;

		// get the parent body
		b2Body* body = shapes[i]->GetBody();

		// get local position
		b2Vec2 localPos;
		switch (shapes[i]->GetType())
		{
		case e_circleShape:		localPos = static_cast<b2CircleShape *>(shapes[i])->GetLocalPosition();	break;
		case e_polygonShape:	localPos = static_cast<b2PolygonShape *>(shapes[i])->GetCentroid(); break;
		default:				localPos = Vector2(0, 0); break;
		}
		Vector2 shapePos(body->GetWorldPoint(localPos));

		// get the collidable identifier
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip non-entity
		if (targetId == 0)
			continue;

		// skip self
		if (targetId == mId)
			continue;

		// get team affiliation
		unsigned int targetTeam = Database::team.Get(targetId);

		// skip neutral
		if (targetTeam == 0)
			continue;

		// skip teammate
		if (targetTeam == aTeam)
			continue;

		// skip indestructible
		if (!Database::damagable.Find(targetId))
			continue;

		// get local direction
		Vector2 localDir(transform.Untransform(shapePos));

		// get range to target
		float range = localDir.Length() - 0.5f * shapes[i]->GetSweepRadius();

		// skip if out of range
		if (range > target.mRange)
			continue;

		// if using a cone angle or angle scale
		if (target.mAngle < float(M_PI)*2.0f || target.mAlign != 0.0f)
		{
			// get angle to target
			float aimAngle = -atan2f(localDir.x, localDir.y);

			// get local angle
			float localAngle = aimAngle - target.mDirection;
			if (localAngle > float(M_PI))
				localAngle -= float(M_PI)*2.0f;
			else if (localAngle < -float(M_PI))
				localAngle += float(M_PI)*2.0f;

			// skip if outside angle
			if (fabsf(localAngle) > target.mAngle)
				continue;

			// if using angle scale...
			if (target.mAlign != 0.0f)
			{
				// apply angle scale
				range *= 1.0f + fabsf(localAngle) * target.mAlign;
			}
		}

		// if the current target...
		if (targetId == mTarget)
		{
			// apply focus scale
			range /= target.mFocus;
		}

		// if better than the current range
		if (bestRange > range)
		{
			// use the new target
			bestRange = range;
			bestTargetId = targetId;
			bestTargetPos = localPos;
		}
	}

	// use the new target
	mTarget = bestTargetId;
	mOffset = bestTargetPos;
	return runningTask;
}

