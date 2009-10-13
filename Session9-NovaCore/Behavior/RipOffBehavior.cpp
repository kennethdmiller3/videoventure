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
		class RipOffBehaviorLoader
		{
		public:
			RipOffBehaviorLoader()
			{
				AddConfigure(0xc4fc7791 /* "ripoff" */, Entry(this, &RipOffBehaviorLoader::Configure));
			}

			~RipOffBehaviorLoader()
			{
				RemoveConfigure(0xc4fc7791 /* "ripoff" */, Entry(this, &RipOffBehaviorLoader::Configure));
			}

			unsigned int Configure(unsigned int aId, const TiXmlElement *element)
			{
				RipOffBehaviorTemplate &ripoff = Database::ripoffbehaviortemplate.Open(aId);
				ripoff.Configure(element, aId);
				Database::ripoffbehaviortemplate.Close(aId);
				return 0x574b0d09 /* "ripoffbehaviortemplate" */;
			}
		}
		ripoffbehaviorloader;
	}

	namespace Initializer
	{
		class RipOffBehaviorInitializer
		{
		public:
			RipOffBehaviorInitializer()
			{
				AddActivate(0x574b0d09 /* "ripoffbehaviortemplate" */, ActivateEntry(this, &RipOffBehaviorInitializer::Activate));
				AddDeactivate(0x574b0d09 /* "ripoffbehaviortemplate" */, DeactivateEntry(this, &RipOffBehaviorInitializer::Deactivate));
			}

			~RipOffBehaviorInitializer()
			{
				RemoveActivate(0x574b0d09 /* "ripoffbehaviortemplate" */, ActivateEntry(this, &RipOffBehaviorInitializer::Activate));
				RemoveDeactivate(0x574b0d09 /* "ripoffbehaviortemplate" */, DeactivateEntry(this, &RipOffBehaviorInitializer::Deactivate));
			}

			Behavior *Activate(unsigned int aId, Controller *aController)
			{
				RipOffBehavior *ripoffbehavior = new RipOffBehavior(aId, aController);
				Database::ripoffbehavior.Put(aId, ripoffbehavior);
				return ripoffbehavior;
			}

			void Deactivate(unsigned int aId)
			{
				if (RipOffBehavior *ripoffbehavior = Database::ripoffbehavior.Get(aId))
				{
					delete ripoffbehavior;
					Database::ripoffbehavior.Delete(aId);
				}
			}
		}
		ripoffbehaviorinitializer;
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
bool RipOffBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	if (const char *tag = element->Attribute("targettag"))
		mTargetTag = Hash(tag);

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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

	// close to an friendly?
	b2Filter filter = { 0xFFFF, 1 << 3, 0 };
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
	b2Filter filter = { 0xFFFF, 1 << 1, 0 };
	unsigned int enemy = ClosestEntity(mTarget ? ripoffbehavior.mAttackRadius : 65536.0f, filter);
	if (!enemy)
		return failedTask;

	// drive towards enemy
	Entity *targetEntity = Database::entity.Get(enemy);
	Vector2 dir = targetEntity->GetPosition() - mEntity->GetPosition();
	Drive(1, dir);

	// check line of fire
	b2Segment shotsegment = { mEntity->GetPosition(), mEntity->GetTransform().Transform(Vector2(0, 64)) };
	b2Filter shotfilter = { 1 << 4, 0xFFFF, 0 };
	float lambda = 1.0f;
	b2Vec2 normal;
	b2Fixture *shape;
	unsigned int hitId = Collidable::TestSegment(shotsegment, shotfilter, mId, lambda, normal, shape);

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
	const b2AABB &boundary = Collidable::GetBoundary();
	float width = boundary.upperBound.x - boundary.lowerBound.x;
	float height = boundary.upperBound.y - boundary.lowerBound.x;
	float length = 2.0f * (width + height);

	// pick a random spot along the perimeter
	float value = Random::Float() * length;

	// map to a position
	Vector2 point;
	if (value <= width)
	{
		point.x = boundary.lowerBound.x + value;
		point.y = boundary.lowerBound.y;
	}
	else
	{
		value -= width;
		if (value <= height)
		{
			point.x = boundary.upperBound.x;
			point.y = boundary.lowerBound.y + value;
		}
		else
		{
			value -= height;
			if (value <= width)
			{
				point.x = boundary.upperBound.x - value;
				point.y = boundary.upperBound.y;
			}
			else
			{
				value -= width;
				point.x = boundary.lowerBound.x;
				point.y = boundary.upperBound.y - value;
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

class ClosestEntityCallback : public b2QueryCallback
{
public:
	unsigned int mId;
	b2Filter mFilter;
	Transform2 mTransform;
	float mBestDist;
	unsigned int mBestTarget;

public:
	ClosestEntityCallback(unsigned int aId, float aRadius, const b2Filter &aFilter, const Transform2 &aTransform)
		: mId(aId), mFilter(aFilter), mTransform(aTransform), mBestDist(aRadius), mBestTarget(0)
	{
	}

	virtual bool ReportFixture(b2Fixture* fixture)
	{
		// skip unhittable fixtures
		if (fixture->IsSensor())
			return true;
		if (!Collidable::CheckFilter(mFilter, fixture->GetFilterData()))
			return true;

		// get the parent body
		b2Body* body = fixture->GetBody();

		// get the collidable identifier
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip non-entity
		if (targetId == 0)
			return true;

		// skip self
		if (targetId == mId)
			return true;

		// get local position
		b2Vec2 localPos;
		switch (fixture->GetType())
		{
		case b2Shape::e_circle:		localPos = static_cast<b2CircleShape *>(fixture->GetShape())->m_p;	break;
		case b2Shape::e_polygon:	localPos = static_cast<b2PolygonShape *>(fixture->GetShape())->m_centroid; break;
		default:					localPos = Vector2(0, 0); break;
		}
		Vector2 fixturePos(body->GetWorldPoint(localPos));

		// get range
		Vector2 dir(mTransform.Transform(fixturePos));
		float dist = dir.Length() - 0.5f * fixture->GetShape()->m_radius;

		// if closer than the current best
		if (mBestDist > dist)
		{
			// update target
			mBestDist = dist;
			mBestTarget = targetId;
		}
		return true;
	}
};

unsigned int RipOffBehavior::ClosestEntity(float aRadius, b2Filter aFilter)
{
	// fill in callback values
	ClosestEntityCallback callback(mId, aRadius, aFilter, mEntity->GetTransform());

	// perform query
	b2AABB aabb;
	aabb.lowerBound.Set(mEntity->GetPosition().x - aRadius, mEntity->GetPosition().y - aRadius);
	aabb.upperBound.Set(mEntity->GetPosition().x + aRadius, mEntity->GetPosition().y + aRadius);
	Collidable::QueryAABB(&callback, aabb);

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
void RipOffBehavior::Escape(unsigned int aId1, unsigned int aId2, float aTime, const b2Contact &aContact)
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
