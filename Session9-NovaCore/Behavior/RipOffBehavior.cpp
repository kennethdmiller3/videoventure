#include "StdAfx.h"

#include "RipOffBehavior.h"
#include "BotUtilities.h"
#include "Tag.h"
#include "Controller.h"
#include "Entity.h"
#include "Link.h"
#include "Collidable.h"


/*
RIP-OFF BEHAVIOR

Sequence
	Pick a random target
	Fly to target
		Aim towards target
		Move towards target
		Succeed when close enough
	Turn around
		Aim away from target
		Succeed when facing directly away
	Attach to target
	Pick a random direction
	Fly in direction
		Aim towards direction
		Move in direction
*/

namespace Database
{
	Typed<RipOffBehaviorTemplate> ripoffbehaviortemplate(0x574b0d09 /* "ripoffbehaviortemplate" */);
	Typed<RipOffBehavior *> ripoffbehavior(0x65e2609b /* "ripoffbehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		static unsigned int RipOffBehaviorConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			RipOffBehaviorTemplate &ripoff = Database::ripoffbehaviortemplate.Open(aId);
			ripoff.Configure(element, aId);
			Database::ripoffbehaviortemplate.Close(aId);
			return 0x574b0d09 /* "ripoffbehaviortemplate" */;
		}
		Configure ripoffbehaviorconfigure(0xc4fc7791 /* "ripoff" */, RipOffBehaviorConfigure);
	}

	namespace Initializer
	{
		static Behavior *RipOffBehaviorActivate(unsigned int aId, Controller *aController)
		{
			RipOffBehavior *ripoffbehavior = new RipOffBehavior(aId, aController);
			Database::ripoffbehavior.Put(aId, ripoffbehavior);
			return ripoffbehavior;
		}
		Activate ripoffbehavioractivate(0x574b0d09 /* "ripoffbehaviortemplate" */, RipOffBehaviorActivate);

		static void RipOffBehaviorDeactivate(unsigned int aId)
		{
			if (RipOffBehavior *ripoffbehavior = Database::ripoffbehavior.Get(aId))
			{
				delete ripoffbehavior;
				Database::ripoffbehavior.Delete(aId);
			}
		}
		Deactivate ripoffbehaviordeactivate(0x574b0d09 /* "ripoffbehaviortemplate" */, RipOffBehaviorDeactivate);
	}
}

// template constructor
RipOffBehaviorTemplate::RipOffBehaviorTemplate(void)
: mTargetTag(0)
, mAvoidRadius(0)
, mAvoidFilter(Collidable::GetDefaultFilter())
, mAttackRadius(0)
, mAttackFilter(Collidable::GetDefaultFilter())
, mCloseRadius(0)
, mCloseScaleDist(0)
, mCloseScaleSpeed(0)
{
}

// configure
bool RipOffBehaviorTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	if (const char *tag = element->Attribute("targettag"))
		mTargetTag = Hash(tag);

	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x2f55069e /* "avoid" */:
			child->QueryFloatAttribute("radius", &mAvoidRadius);
			ConfigureFilterData(mAvoidFilter, child);
			break;

		case 0x4595b8fd /* "attack" */:
			child->QueryFloatAttribute("radius", &mAttackRadius);
			ConfigureFilterData(mAttackFilter, child);
			break;

		case 0x27cb3b23 /* "close" */:
			child->QueryFloatAttribute("radius", &mCloseRadius);
			child->QueryFloatAttribute("scaledist", &mCloseScaleDist);
			child->QueryFloatAttribute("scalespeed", &mCloseScaleSpeed);
			break;
		}
	}
	return true;
}

// constructor
RipOffBehavior::RipOffBehavior(unsigned int aId, Controller *aController)
: Behavior(aId, aController)
, mEntity(Database::entity.Get(aId))
, mTarget(0)
, mDirection(0, 0)
{
	// start by picking the closest target
	bind(this, &RipOffBehavior::PickClosestTarget);

	// add a boundary violation listener: escape
	Collidable::ContactSignal &signal = Database::collidablecontactadd.Open(0x9e212406 /* "escape" */);
	signal.Connect(this, &RipOffBehavior::Escape);
	Database::collidablecontactadd.Close(mId);
}

// destructor
RipOffBehavior::~RipOffBehavior()
{
	// detach and unlock any target
	if (mTarget)
	{
		Detach(mTarget);
		Unlock(mTarget);
	}

	// remove boundary violation listener
	Collidable::ContactSignal &signal = Database::collidablecontactadd.Open(0x9e212406 /* "escape" */);
	signal.Disconnect(this, &RipOffBehavior::Escape);
	Database::collidablecontactadd.Close(mId);
}

// pick a random target
Status RipOffBehavior::PickRandomTarget(void)
{
	// unlock current target
	if (mTarget)
	{
		Unlock(mTarget);
		mTarget = 0;
	}

	// get a random target
	mTarget = RandomTarget();

	// try again if no targets to pick
	if (!mTarget)
	{
		// higher-priority tasks
		if (AvoidFriendly() != failedTask)
			return runningTask;
		if (AttackEnemy() != failedTask)
			return runningTask;
		return runningTask;
	}

	// lock it
	Lock(mTarget);

	// go to next sub-behavior
	return Task(this, &RipOffBehavior::FlyToTarget);
}

// pick the closest target
Status RipOffBehavior::PickClosestTarget(void)
{
	// unlock current target
	if (mTarget)
	{
		Unlock(mTarget);
		mTarget = 0;
	}

	// closest target
	mTarget = ClosestTarget(65536.0f, false);

	// try again if no targets to pick
	if (!mTarget)
	{
		// higher-priority tasks
		if (AvoidFriendly() != failedTask)
			return runningTask;
		if (AttackEnemy() != failedTask)
			return runningTask;
		return runningTask;
	}

	// lock it
	Lock(mTarget);

	// go to next sub-behavior
	return Task(this, &RipOffBehavior::FlyToTarget);
}

Status RipOffBehavior::AvoidFriendly(void)
{
	const RipOffBehaviorTemplate &ripoffbehavior = Database::ripoffbehaviortemplate.Get(mId);

	// close to a friendly?
	CollidableFilter filter(0, ~0U, 1U<<3);
	unsigned int friendly = ClosestEntity(ripoffbehavior.mAvoidRadius, filter);
	if (!friendly)
		return failedTask;

	// drive away from friendly
	Entity *targetEntity = Database::entity.Get(friendly);
	Vector2 dir = targetEntity->GetPosition() - mEntity->GetPosition();
	Drive(1, -dir);

	// done
	return completedTask;
}

Status RipOffBehavior::AttackEnemy(void)
{
	const RipOffBehaviorTemplate &ripoffbehavior = Database::ripoffbehaviortemplate.Get(mId);

	// close to an enemy?
	CollidableFilter filter(0, ~0U, 1U<<1);
	unsigned int enemy = ClosestEntity(mTarget ? ripoffbehavior.mAttackRadius : 65536.0f, filter);
	if (!enemy)
		return failedTask;

	// drive towards enemy
	Entity *targetEntity = Database::entity.Get(enemy);
	Vector2 dir = targetEntity->GetPosition() - mEntity->GetPosition();
	Drive(1, dir);

	// check line of fire
	Vector2 shotstart = mEntity->GetPosition();
	Vector2 shotend = mEntity->GetTransform().Transform(Vector2(0, 64));
	CollidableFilter shotfilter(0, 1U<<4, ~0U);
	float lambda = 1.0f;
	Vector2 normal;
	CollidableShape *shape;
	unsigned int hitId = Collidable::TestSegment(shotstart, shotend, shotfilter, mId, lambda, normal, shape);

	// fire if not blocked
	if (hitId == 0 || hitId == enemy)
		mController->mFire[0] = 1;

	// done
	return completedTask;
}

// fly to the target
Status RipOffBehavior::FlyToTarget(void)
{
	// higher-priority tasks
	if (AvoidFriendly() != failedTask)
		return runningTask;
	if (AttackEnemy() != failedTask)
		return runningTask;

	const RipOffBehaviorTemplate &ripoffbehavior = Database::ripoffbehaviortemplate.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (!targetEntity)
		return Task(this, &RipOffBehavior::PickClosestTarget);

	// get direction and distance to target
	Vector2 dir = targetEntity->GetPosition() - mEntity->GetPosition();
	float dist = dir.Length();
	dir /= dist;

	// get target relative speed
	Vector2 vel = targetEntity->GetVelocity() - mEntity->GetVelocity();
	float speed = vel.Dot(dir);

	// scale based on distance
	float scale = (dist - ripoffbehavior.mCloseRadius) * ripoffbehavior.mCloseScaleDist + speed * ripoffbehavior.mCloseScaleSpeed;

	// drive
	Drive(scale, dir);

	// keep running if not close enough
	if (dist > ripoffbehavior.mCloseRadius + 2)
		return runningTask;

	// go to next sub-behavior
	return Task(this, &RipOffBehavior::TurnAround);
}

// turn around
Status RipOffBehavior::TurnAround(void)
{
	// get target entity
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (!targetEntity)
		return Task(this, &RipOffBehavior::PickClosestTarget);

	const RipOffBehaviorTemplate &ripoffbehavior = Database::ripoffbehaviortemplate.Get(mId);

	// get direction and distance to target
	Vector2 dir = targetEntity->GetPosition() - mEntity->GetPosition();
	float dist = dir.Length();
	dir /= dist;

	// drive
	Drive(0, -dir);

	// keep running if not close enough
	if (dist > ripoffbehavior.mCloseRadius + 2)
		return Task(this, &RipOffBehavior::FlyToTarget);

	// keep running if not aiming away
	const Transform2 &transform = mEntity->GetTransform();
	if (transform.Unrotate(dir).y > -63.0f/64.0f)
		return runningTask;

	// go to next sub-behavior
	return Task(this, &RipOffBehavior::AttachToTarget);
}

// attach to the target
Status RipOffBehavior::AttachToTarget(void)
{
	// get target entity
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (!targetEntity)
		return Task(this, &RipOffBehavior::PickClosestTarget);

	// attach it
	Attach(mTarget);

	// go to the next sub-behavior
	return Task(this, &RipOffBehavior::PickRandomDirection);
}

// pick a random escape direction
Status RipOffBehavior::PickRandomDirection(void)
{
	// get world boundary perimeter
	const AlignedBox2 &boundary = Collidable::GetBoundary();
	float width = boundary.max.x - boundary.min.x;
	float height = boundary.max.y - boundary.min.x;
	float length = 2.0f * (width + height);

	// pick a random spot along the perimeter
	float value = Random::Float() * length;

	// map to a position
	Vector2 point;
	if (value <= width)
	{
		point.x = boundary.min.x + value;
		point.y = boundary.min.y;
	}
	else
	{
		value -= width;
		if (value <= height)
		{
			point.x = boundary.max.x;
			point.y = boundary.min.y + value;
		}
		else
		{
			value -= height;
			if (value <= width)
			{
				point.x = boundary.max.x - value;
				point.y = boundary.max.y;
			}
			else
			{
				value -= width;
				point.x = boundary.min.x;
				point.y = boundary.max.y - value;
			}
		}
	}

	// get direction
	mDirection = point - mEntity->GetPosition();
	mDirection /= mDirection.Length();

	// go to the next sub-behavior
	return Task(this, &RipOffBehavior::FlyInDirection);
}

// fly in the escape direction
Status RipOffBehavior::FlyInDirection(void)
{
	// higher-priority tasks
	if (AvoidFriendly() != failedTask)
		return runningTask;

	// get target entity
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (!targetEntity)
		return Task(this, &RipOffBehavior::PickClosestTarget);

	// drive
	Drive(1, mDirection);

	return runningTask;
}

unsigned int RipOffBehavior::RandomTarget(void)
{
	const RipOffBehaviorTemplate &ripoffbehavior = Database::ripoffbehaviortemplate.Get(mId);

	// target candidates
	std::vector<unsigned int> targets;

	// for each entity...
	for (Database::Typed<Entity *>::Iterator itor(&Database::entity); itor.IsValid(); ++itor)
	{
		// get the entity's identifier
		unsigned int id = itor.GetKey();

		// if tagged as stealable
		if (Database::tag.Get(id).Get(ripoffbehavior.mTargetTag).i)
		{
			// if not locked by another unit
			if (!GetLocked(id))
			{
				// add to target list
				targets.push_back(id);
			}
		}
	}

	// no targets?
	if (targets.empty())
		return 0;

	// pick one of them at random
	int index = Random::Int() % targets.size();
	return targets[index];
}

unsigned int RipOffBehavior::ClosestTarget(float aRadius, bool aLocked)
{
	const RipOffBehaviorTemplate &ripoffbehavior = Database::ripoffbehaviortemplate.Get(mId);

	// owner position
	const Vector2 &position = mEntity->GetPosition();

	// best distance
	float bestDistSq = aRadius * aRadius;

	// best target
	unsigned int bestTarget = 0;

	// for each entity...
	for (Database::Typed<Entity *>::Iterator itor(&Database::entity); itor.IsValid(); ++itor)
	{
		// get the entity's identifier
		unsigned int id = itor.GetKey();

		// if tagged as stealable
		if (Database::tag.Get(id).Get(ripoffbehavior.mTargetTag).i)
		{
			// if overriding locked, or not locked by another unit...
			if (aLocked || !GetLocked(id))
			{
				// get target entity
				Entity *targetEntity = Database::entity.Get(id);

				// add to target list
				float distSq = position.DistSq(targetEntity->GetPosition());
				if (bestDistSq > distSq)
				{
					bestDistSq = distSq;
					bestTarget = id;
				}
			}
		}
	}

	// return the best target
	return bestTarget;
}

class ClosestEntityCallback
{
public:
	unsigned int mId;
	CollidableFilter mFilter;
	Transform2 mTransform;
	float mBestDist;
	unsigned int mBestTarget;

public:
	ClosestEntityCallback(unsigned int aId, float aRadius, const CollidableFilter &aFilter, const Transform2 &aTransform)
		: mId(aId), mFilter(aFilter), mTransform(aTransform), mBestDist(aRadius), mBestTarget(0)
	{
	}

	void Report(CollidableShape *shape, float distance, const Vector2 &point)
	{
		// skip unhittable shapes
		if (Collidable::IsSensor(shape))
			return;
		if (!Collidable::CheckFilter(mFilter, Collidable::GetFilter(shape)))
			return;

		// get the collidable identifier
		unsigned int targetId = Collidable::GetId(shape);

		// skip non-entity
		if (targetId == 0)
			return;

		// skip self
		if (targetId == mId)
			return;

		// if closer than the current best
		if (mBestDist > distance)
		{
			// update target
			mBestDist = distance;
			mBestTarget = targetId;
		}
	}
};

unsigned int RipOffBehavior::ClosestEntity(float aRadius, const CollidableFilter &aFilter)
{
	// fill in callback values
	ClosestEntityCallback callback(mId, aRadius, aFilter, mEntity->GetTransform());

	// perform query
	Collidable::QueryRadius(mEntity->GetPosition(), aRadius, aFilter, 
		Collidable::QueryRadiusDelegate(&callback, &ClosestEntityCallback::Report));

	// return best target
	return callback.mBestTarget;
}

// drive
void RipOffBehavior::Drive(float aSpeed, Vector2 aDirection)
{
	// get transform
	const Transform2 &transform = mEntity->GetTransform();

	// move at specified speed
	mController->mMove += transform.Rotate(Vector2(0.0f, aSpeed));

	// local direction
	Vector2 localDir = transform.Unrotate(aDirection);

	// turn towards target direction
	mController->mTurn += SteerTowards(mEntity, localDir);
}

// escape
void RipOffBehavior::Escape(unsigned int aId1, unsigned int aId2, float aTime, const Vector2 &aContact, const Vector2 &aNormal)
{
	// skip incorrect recipient
	if (aId2 != mId)
		return;

	// I'm free!  I'm freeeeee!
	DebugPrint("\"%s\" (%08x) escaped with \"%s\" (%08x)\n", Database::name.Get(mId).c_str(), mId, Database::name.Get(mTarget).c_str(), mTarget);
	Database::Delete(mTarget);
	Database::Delete(mId);
}

// attach the specified target
void RipOffBehavior::Attach(unsigned int aTarget)
{
	assert(mTarget == aTarget);
	DebugPrint("\"%s\" (%08x) attached \"%s\" (%08x)\n", Database::name.Get(mId).c_str(), mId, Database::name.Get(aTarget).c_str(), aTarget);

	const RipOffBehaviorTemplate &ripoffbehavior = Database::ripoffbehaviortemplate.Get(mId);

	// create a link template
	Database::Typed<LinkTemplate> &linktemplates = Database::linktemplate.Open(mId);
	LinkTemplate &linktemplate = linktemplates.Open(mTarget);
	linktemplate.mOffset = Transform2(0, Vector2(0, -ripoffbehavior.mCloseRadius));
	linktemplate.mSub = mTarget;
	linktemplate.mSecondary = mTarget;
	linktemplate.mUpdateAngle = false;
	linktemplate.mDeleteSecondary = false;

	// create a link
	Link *link = new Link(linktemplate, mId);
	Database::Typed<Link *> &links = Database::link.Open(mId);
	links.Put(mTarget, link);
	Database::link.Close(mId);
	link->Activate();

	linktemplates.Close(mTarget);
	Database::linktemplate.Close(mId);
}

// detach the specified target
void RipOffBehavior::Detach(unsigned int aTarget)
{
	assert(mTarget == aTarget);
	if (Database::link.Find(mId))
	{
		DebugPrint("\"%s\" (%08x) detached \"%s\" (%08x)\n", Database::name.Get(mId).c_str(), mId, Database::name.Get(aTarget).c_str(), aTarget);

		// remove the link
		Database::Typed<Link *> &links = Database::link.Open(mId);
		Link *link = links.Get(mTarget);
		if (link)
		{
			link->Deactivate();
			delete link;
		}
		links.Delete(mTarget);
		Database::link.Close(mId);
	}

	if (Database::linktemplate.Find(mId))
	{
		// remove the link template
		Database::Typed<LinkTemplate> &linktemplates = Database::linktemplate.Open(mId);
		linktemplates.Delete(mTarget);
		Database::linktemplate.Close(mId);
	}
}

// lock the specified target
void RipOffBehavior::Lock(unsigned int aTarget)
{
	assert(mTarget == aTarget);
	DebugPrint("\"%s\" (%08x) locked \"%s\" (%08x)\n", Database::name.Get(mId).c_str(), mId, Database::name.Get(mTarget).c_str(), mTarget);

	// set the "locked" tag to the owner
	Database::Typed<Tag> &tags = Database::tag.Open(aTarget);
	Tag &tag = tags.Open(0xce164093 /* "locked" */);
	tag.u = mId;
	tags.Close(0xce164093 /* "locked" */);
	Database::tag.Close(aTarget);
}

// unlock the specified target
void RipOffBehavior::Unlock(unsigned int aTarget)
{
	assert(mTarget == aTarget);
	if (Database::tag.Find(aTarget))
	{
		DebugPrint("\"%s\" (%08x) unlocked \"%s\" (%08x)\n", Database::name.Get(mId).c_str(), mId, Database::name.Get(mTarget).c_str(), mTarget);

		// remove the "locked" tag
		Database::Typed<Tag> &tags = Database::tag.Open(aTarget);
		if (tags.Get(0xce164093 /* "locked" */).u == mId)
			tags.Delete(0xce164093 /* "locked" */);
		Database::tag.Close(aTarget);
	}
}

// get the lock owner (if any) for the specified target
unsigned int RipOffBehavior::GetLocked(unsigned int aTarget)
{
	return Database::tag.Get(aTarget).Get(0xce164093 /* "locked" */).u;
}
