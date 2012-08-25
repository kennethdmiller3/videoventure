#include "StdAfx.h"

#include "RipOffBehavior.h"
#include "BotUtilities.h"
#include "Tag.h"
#include "..\Controller.h"
#include "..\Entity.h"
#include "..\Ship.h"
#include "..\Link.h"
#include "..\Collidable.h"


/*
RIP-OFF BEHAVIOR

Tim Skelly
Rip-Off: A Description of Programmed Behavior in a Video Game
http://www.red3d.com/cwr/boids/RipOff_Flocking.html

	1. THIS IS WHAT YOU WANT: When initialized, the attacking tanks were each
		given a particular canister to retrieve. If there were more tanks than
		canisters, the extra tanks were put into "attack mode," which I will
		describe later.
	2. GO GET IT: Each tank moved towards their assigned canister in the
		fashion of the ships in "Tailgunner." They moved forward while turning
		towards their target position, decelerating when near their targeted
		canister so as to prevent "orbiting."
	3. HOOK IT TO YOUR TRAILER HITCH: When close enough to do so, an enemy tank
		would stop, rotate and attach the target canister to its tail end.
	4. RUN FOR IT: Once attached to a canister, the tank was assigned a random
		exit point to accelerate towards. On reaching the exit, the tank and
		canister were removed from play.
	5. DON'T BUMP YOUR BUDDIES: When within a certain radius of another tank,
		each tank was to steer away from the other until out of range. (This
		was also used to initialize the tanks on entry. I placed each tank on
		the same spot and then let their avoidance A.I. and seeking behavior
		disperse them.)
	6. GET AWAY FROM ME OR DIE: One behavior had priority over canister
		seeking and removal, but not avoidance. When within a certain radius of
		a player figure, if not attached to a canister, the enemies were
		programmed to make the player's tank their goal and to fire a short-
		range "beam." If that beam hit the player's tank, as in a collision,
		the player figure was removed from the screen for a few seconds, then
		placed back at its starting position. (Unlike the behavior when
		approaching a canister, when in attack mode the enemy ships did not
		decelerate when they neared their target. This meant that clever
		players could, and often did, allow enemy tanks to orbit them. This
		effectively put the game on "pause.")
	7. IS THIS YOURS? TAKE MINE: As someone once said, the trick to making
		computers appear intelligent is simply to make them not do stupid
		things. An enemy tank that would drive right over an available canister
		on its way to the one assigned to it would look stupid. When within a
		certain radius of ANY canister not attached or in the process of being
		attached, enemy tanks were programmed to make the nearby canister their
		target canister and to go immediately into "pick-up" mode. Before doing
		so, the tank would check to see if the canister it was now attaching
		had been assigned to another tank. If so, it would swap canister
		assignments with that tank.

Prioritized behaviors:
	1. If no valid target...
		Select a target (random/closest?)
	2. If within range of a friendly unit...
		Steer away from the unit
	3. If attached to a stealable...
		Steer towards escape point
		Speed at full
	4. If within range of an enemy unit...
		Steer towards the unit
		Trigger weapons
	5. If within range of an non-targeted stealable...
		If locked by another unit...
			Reassign current target to that unit
		Target the stealable
	6. If within range of the target and pointing away...
		Attach to target
		Pick a random escape point
	7. If within range of the target and not pointing away
		Steer away from target
		Speed at zero
	8. Move towards target
		Steer towards target
		Speed based on distance
*/

// tuning parameters
// TO DO: put in a template class
const float AVOID_RADIUS = 24;
const float ATTACK_RADIUS = 32;
const float TARGET_RADIUS = 32;
const float CLOSE_RADIUS = 12;

unsigned int RipOffBehavior::RandomTarget(void)
{
	// target candidates
	std::vector<unsigned int> targets;

	// for each entity...
	for (Database::Typed<Entity *>::Iterator itor(&Database::entity); itor.IsValid(); ++itor)
	{
		// get the entity's identifier
		unsigned int id = itor.GetKey();

		// if tagged as stealable
		if (Database::tag.Get(id).Get(0x19e80868 /* "stealable" */).i)
		{
			// if not locked by another unit
			if (!GetLocked(id))
			{
				// add to target list
				targets.push_back(id);
			}
		}
	}

	// try again if no targets to pick
	if (targets.empty())
		return failedTask();

	// pick one of them at random
	int index = Random::Int() % targets.size();
	return targets[index];
}

unsigned int RipOffBehavior::ClosestTarget(float aRadius, bool aLocked)
{
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
		if (Database::tag.Get(id).Get(0x19e80868 /* "stealable" */).i)
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

Status RipOffBehavior::SelectTarget(void)
{
	// complete if already targeted
	if (mTarget)
		return completedTask();

	// pick a random target
	mTarget = RandomTarget();

	// fail if nothing found
	if (!mTarget)
		return failedTask();

	// lock the target
	Lock(mTarget);

	// complete
	return completedTask();
}

Status RipOffBehavior::AvoidFriendly(void)
{
	// friendly-unit filter (HACK)
	b2FilterData filter;
	filter.categoryBits = 0xFFFF;
	filter.maskBits = 1 << 3;
	filter.groupIndex = 0;

	// get the collision world
	b2World *world = Collidable::GetWorld();

	// get nearby shapes
	b2AABB aabb;
	const float lookRadius = AVOID_RADIUS;
	aabb.lowerBound.Set(mEntity->GetPosition().x - lookRadius, mEntity->GetPosition().y - lookRadius);
	aabb.upperBound.Set(mEntity->GetPosition().x + lookRadius, mEntity->GetPosition().y + lookRadius);
	b2Shape* shapes[b2_maxProxies];
	int32 count = world->Query(aabb, shapes, b2_maxProxies);

	// get transform
	const Matrix2 transform(mEntity->GetTransform());

	// avoided anything?
	bool avoided = false;

	// for each shape...
	for (int32 i = 0; i < count; ++i)
	{
		// get the shape
		b2Shape* shape = shapes[i];

		// skip unhittable shapes
		if (shape->IsSensor())
			continue;
		if (!Collidable::CheckFilter(filter, shape->GetFilterData()))
			continue;

		// get the parent body
		b2Body* body = shapes[i]->GetBody();

		// get the collidable identifier
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip non-entity
		if (targetId == 0)
			continue;

		// skip self
		if (targetId == mId)
			continue;

		// get range
		Vector2 dir(transform.Untransform(Vector2(body->GetPosition())));
		float range = dir.Length() - 0.5f * shapes[i]->GetSweepRadius();

		// skip if out of range
		if (range > AVOID_RADIUS)
			continue;

		// avoid strength
		float avoid = range / AVOID_RADIUS;

		// move forward
		mController->mMove = transform.y;

		// steer away
		mController->mTurn += dir.x > 0.0f ? -avoid : avoid;

		// set avoided
		avoided = true;
	}

	return avoided ? completedTask() : failedTask();
}

Status RipOffBehavior::SeekEscape(void)
{
	if (!mAttached)
		return failedTask();

	// get transform
	const Transform2 &transform = mEntity->GetTransform();

	// turn towards escape direction
	Vector2 dir(transform.Unrotate(mDirection));
	mController->mTurn += dir.x > 0.0f ? 1.0f : -1.0f;

	// maximum speed
	mController->mMove = transform.Rotate(Vector2(0, 1));

	return completedTask();
}

Status RipOffBehavior::AttackEnemy(void)
{
	// enemy-unit filter (HACK)
	b2FilterData filter;
	filter.categoryBits = 0xFFFF;
	filter.maskBits = 1 << 1;
	filter.groupIndex = 0;

	// get transform matrix
	const Matrix2 transform(mEntity->GetTransform());

	// get the collision world
	b2World *world = Collidable::GetWorld();

	// get nearby shapes
	b2AABB aabb;
	const float lookRadius = mTarget ? ATTACK_RADIUS : FLT_MAX;
	aabb.lowerBound.Set(mEntity->GetPosition().x - lookRadius, mEntity->GetPosition().y - lookRadius);
	aabb.upperBound.Set(mEntity->GetPosition().x + lookRadius, mEntity->GetPosition().y + lookRadius);
	b2Shape* shapes[b2_maxProxies];
	int32 count = world->Query(aabb, shapes, b2_maxProxies);

	// attacked anything?
	bool attacked = false;

	// for each shape...
	for (int32 i = 0; i < count; ++i)
	{
		// get the shape
		b2Shape* shape = shapes[i];

		// skip unhittable shapes
		if (shape->IsSensor())
			continue;
		if (!Collidable::CheckFilter(filter, shape->GetFilterData()))
			continue;

		// get the parent body
		b2Body* body = shapes[i]->GetBody();

		// get the collidable identifier
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip non-entity
		if (targetId == 0)
			continue;

		// skip self
		if (targetId == mId)
			continue;

		// get range
		Vector2 dir(transform.Untransform(body->GetPosition()));
		float range = dir.Length() - 0.5f * shapes[i]->GetSweepRadius();

		// skip if out of range
		if (range > ATTACK_RADIUS)
			continue;

		// attack strength
		float attack = range / ATTACK_RADIUS;

		// move forward
		mController->mMove = transform.y;

		// steer towards
		mController->mTurn = dir.x > 0.0f ? attack : -attack;

		// fire
		mController->mFire[0] = attack;

		// set attacked
		attacked = true;
	}

	return attacked ? completedTask() : failedTask();
}

Status RipOffBehavior::NearStealable(void)
{
	return failedTask();
}

Status RipOffBehavior::NearTarget(void)
{
	// get transform
	const Transform2 &transform = mEntity->GetTransform();

	// get target entity
	Entity *targetEntity = Database::entity.Get(mTarget);

	// 
}

Status RipOffBehavior::SeekTarget(void)
{
}

// constructor
RipOffBehavior::RipOffBehavior(unsigned int aId, Controller *aController)
: Behavior(aId, aController)
, mTarget(0)
, mDirection(0, 0)
{
	bind(this, &RipOffBehavior::Execute);

	// parent entity
	mEntity = Database::entity.Get(mId);

	// add a boundary violation listener: escape
	Database::Typed<Collidable::BoundaryListener> &collidableboundaryviolations = Database::collidableboundaryviolation.Open(mId);
	collidableboundaryviolations.Put(Database::Key(this), Collidable::BoundaryListener(this, &RipOffBehavior::Escape));
	Database::collidableboundaryviolation.Close(mId);
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
	if (Database::collidableboundaryviolation.Find(mId))
	{
		Database::Typed<Collidable::BoundaryListener> &collidableboundaryviolations = Database::collidableboundaryviolation.Open(mId);
		collidableboundaryviolations.Delete(Database::Key(this));
		Database::collidableboundaryviolation.Close(mId);
	}
}

Status RipOffBehavior::Execute(void)
{
	if (SelectTarget() == completedTask())

}

// fly to the target
Status RipOffBehavior::FlyToTarget(void)
{
	// get target entity
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (!targetEntity)
		return Task(this, &RipOffBehavior::PickClosestTarget);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get direction and distance to target
	Vector2 dir = targetEntity->GetPosition() - entity->GetPosition();
	float dist = dir.Length();
	dir /= dist;

	// get target relative speed
	Vector2 vel = targetEntity->GetVelocity() - entity->GetVelocity();
	float speed = vel.Dot(dir);

	// move towards target position
	float scale = (dist - CLOSE_RANGE) * CLOSE_SCALEDIST + speed * CLOSE_SCALESPEED;
	mController->mMove += dir * scale;

	// local direction
	Vector2 localDir = transform.Unrotate(dir);

	// angle to target direction
	float aimAngle = -atan2f(localDir.x, localDir.y);

	// turn towards target direction
	const ShipTemplate &ship = Database::shiptemplate.Get(mId);	// <-- hack!
	if (ship.mMaxOmega != 0.0f)
	{
		mController->mTurn += Clamp(aimAngle / (ship.mMaxOmega * sim_step), -1.0f, 1.0f);
	}

	// keep running if not close enough
	if (dist > CLOSE_RANGE + 1)
		return runningTask();

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

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get direction and distance to target
	Vector2 dir = targetEntity->GetPosition() - entity->GetPosition();
	float dist = dir.Length();
	dir /= dist;

	// get target relative speed
	Vector2 vel = targetEntity->GetVelocity() - entity->GetVelocity();
	float speed = vel.Dot(dir);

	// move towards target position
	float scale = (dist - CLOSE_RANGE) * CLOSE_SCALEDIST + speed * CLOSE_SCALESPEED;
	mController->mMove += dir * scale;

	// local direction
	Vector2 localDir = -transform.Unrotate(dir);

	// angle to target direction
	float aimAngle = -atan2f(localDir.x, localDir.y);

	// turn towards target direction
	const ShipTemplate &ship = Database::shiptemplate.Get(mId);	// <-- hack!
	if (ship.mMaxOmega != 0.0f)
	{
		mController->mTurn += Clamp(aimAngle / (ship.mMaxOmega * sim_step), -1.0f, 1.0f);
	}

	// keep running if not close enough
	if (dist > CLOSE_RANGE + 1)
		return runningTask();

	// keep running if not aiming away
	if (fabsf(aimAngle) > 1.0f/64.0f)
		return runningTask();

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
	// random unit vector
	float angle = Random::Float() * float(M_PI) * 2.0f;
	mDirection.x = cosf(angle);
	mDirection.y = sinf(angle);

	// go to the next sub-behavior
	return Task(this, &RipOffBehavior::FlyInDirection);
}

// fly in the escape direction
Status RipOffBehavior::FlyInDirection(void)
{
	// get target entity
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (!targetEntity)
		return Task(this, &RipOffBehavior::PickClosestTarget);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// floor it!
	mController->mMove += transform.Rotate(Vector2(0, 1));

	// local direction
	Vector2 localDir = transform.Unrotate(mDirection);

	// angle to target direction
	float aimAngle = -atan2f(localDir.x, localDir.y);

	// turn towards target direction
	const ShipTemplate &ship = Database::shiptemplate.Get(mId);	// <-- hack!
	if (ship.mMaxOmega != 0.0f)
	{
		mController->mTurn += Clamp(aimAngle / (ship.mMaxOmega * sim_step), -1.0f, 1.0f);
	}

	return runningTask();
}

// escape
void RipOffBehavior::Escape(unsigned int aId)
{
	assert(aId == mId);

	// I'm free!  I'm freeeeee!
	Database::Delete(mTarget);
	Database::Delete(mId);
}

// attach the specified target
void RipOffBehavior::Attach(unsigned int aTarget)
{
	// create a link template
	Database::Typed<LinkTemplate> &linktemplates = Database::linktemplate.Open(mId);
	LinkTemplate &linktemplate = linktemplates.Open(mTarget);
	linktemplate.mOffset = Transform2(0, Vector2(0, -CLOSE_RANGE));
	linktemplate.mSub = mTarget;
	linktemplate.mSecondary = mTarget;
	linktemplate.mUpdateAngle = false;
	linktemplate.mUpdateTeam = false;
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
	if (Database::link.Find(mId))
	{
		// remove the link
		Database::Typed<Link *> &links = Database::link.Open(mId);
		delete links.Get(mTarget);
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
	if (Database::tag.Find(aTarget))
	{
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
