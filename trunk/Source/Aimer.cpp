#include "StdAfx.h"
#include "Aimer.h"
#include "Collidable.h"
#include "Entity.h"
#include "Link.h"
#include "Weapon.h"
#include "Damagable.h"
#include "Team.h"

#include "Ship.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// aimer pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Aimer));
void *Aimer::operator new(size_t aSize)
{
	return pool.malloc();
}
void Aimer::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<AimerTemplate> aimertemplate(0x9bde0ae7 /* "aimertemplate" */);
	Typed<Aimer *> aimer(0x2ea90881 /* "aimer" */);

	namespace Loader
	{
		class AimerLoader
		{
		public:
			AimerLoader()
			{
				AddConfigure(0x2ea90881 /* "aimer" */, Entry(this, &AimerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				AimerTemplate &aimer = Database::aimertemplate.Open(aId);
				aimer.Configure(element);
				Database::aimertemplate.Close(aId);
			}
		}
		aimerloader;
	}

	namespace Initializer
	{
		class AimerInitializer
		{
		public:
			AimerInitializer()
			{
				AddActivate(0x9bde0ae7 /* "aimertemplate" */, Entry(this, &AimerInitializer::Activate));
				AddDeactivate(0x9bde0ae7 /* "aimertemplate" */, Entry(this, &AimerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const AimerTemplate &aimertemplate = Database::aimertemplate.Get(aId);
				Aimer *aimer = new Aimer(aimertemplate, aId);
				Database::aimer.Put(aId, aimer);
				Database::controller.Put(aId, aimer);
				aimer->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Aimer *aimer = Database::aimer.Get(aId))
				{
					delete aimer;
					Database::aimer.Delete(aId);
					Database::controller.Delete(aId);
				}
			}
		}
		aimerinitializer;
	}
}

WanderBehaviorTemplate::WanderBehaviorTemplate()
: mSide(0.0f)
, mSideRate(0.0f)
, mFront(0.0f)
, mFrontRate(0.0f)
, mTurn(0.0f)
, mTurnRate(0.0f)
{
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

PursueBehaviorTemplate::PursueBehaviorTemplate()
: mStrength(0.0f)
, mLeading(0.0f)
{
}

AimBehaviorTemplate::AimBehaviorTemplate()
: mStrength(0.0f)
, mLeading(0.0f)
{
}

FireBehaviorTemplate::FireBehaviorTemplate()
: mRange(0.0f)
, mDirection(0.0f)
, mAngle(0.3f)
, mChannel(-1)
{
}

EvadeBehaviorTemplate::EvadeBehaviorTemplate()
: mStrength(0.0f)
{
}

CloseBehaviorTemplate::CloseBehaviorTemplate()
: mRange(-FLT_MAX)
, mScaleDist(1.0f/16.0f)
, mScaleSpeed(0.0f)
{
}

FarBehaviorTemplate::FarBehaviorTemplate()
: mRange(FLT_MAX)
, mScaleDist(1.0f/64.0f)
, mScaleSpeed(0.0f)
{
}

AimerTemplate::AimerTemplate(void)
: mDrift(0.0f)
, mFire(NULL)
, mFireCount(0)
{
}

AimerTemplate::~AimerTemplate(void)
{
}

bool AimerTemplate::Configure(const TiXmlElement *element)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x2e87eea4 /* "drift" */:
			{
				child->QueryFloatAttribute("strength", &mDrift);
			}
			break;

		case 0xf23b7114 /* "wander" */:
			{
				child->QueryFloatAttribute("side", &mWander.mSide);
				child->QueryFloatAttribute("siderate", &mWander.mSideRate);
				child->QueryFloatAttribute("front", &mWander.mFront);
				child->QueryFloatAttribute("frontrate", &mWander.mFrontRate);
				child->QueryFloatAttribute("turn", &mWander.mTurn);
				child->QueryFloatAttribute("turnrate", &mWander.mTurnRate);
			}
			break;

		case 0x32608848 /* "target" */:
			{
				child->QueryFloatAttribute("period", &mTarget.mPeriod);
				child->QueryFloatAttribute("range", &mTarget.mRange);
				child->QueryFloatAttribute("direction", &mTarget.mDirection);
				child->QueryFloatAttribute("angle", &mTarget.mAngle);
				child->QueryFloatAttribute("focus", &mTarget.mFocus);
				child->QueryFloatAttribute("align", &mTarget.mAlign);
				ConfigureFilterData(mTarget.mFilter, child);
			}
			break;

		case 0x0297228f /* "pursue" */:
			{
				child->QueryFloatAttribute("strength", &mPursue.mStrength);
				child->QueryFloatAttribute("leading", &mPursue.mLeading);
			}
			break;

		case 0x383251f6 /* "aim" */:
			{
				child->QueryFloatAttribute("strength", &mAim.mStrength);
				child->QueryFloatAttribute("leading", &mAim.mLeading);
			}
			break;

		case 0x8eab16d9 /* "fire" */:
			{
				mFire = static_cast<FireBehaviorTemplate *>(realloc(mFire, (mFireCount + 1) * sizeof(FireBehaviorTemplate)));
				new (&mFire[mFireCount]) FireBehaviorTemplate();
				if (child->QueryIntAttribute("channel", &mFire[mFireCount].mChannel) == TIXML_SUCCESS)
					--mFire[mFireCount].mChannel;
				child->QueryFloatAttribute("range", &mFire[mFireCount].mRange);
				if (child->QueryFloatAttribute("direction", &mFire[mFireCount].mDirection) == TIXML_SUCCESS)
					mFire[mFireCount].mDirection *= float(M_PI) / 180.0f;
				if (child->QueryFloatAttribute("angle", &mFire[mFireCount].mAngle) == TIXML_SUCCESS)
					mFire[mFireCount].mAngle *= float(M_PI) / 180.0f;
				++mFireCount;
			}
			break;

		case 0x3cf27f66 /* "evade" */:
			{
				child->QueryFloatAttribute("strength", &mEvade.mStrength);
			}
			break;

		case 0x27cb3b23 /* "close" */:
			{
				child->QueryFloatAttribute("range", &mClose.mRange);
				child->QueryFloatAttribute("scaledist", &mClose.mScaleDist);
				child->QueryFloatAttribute("scalespeed", &mClose.mScaleSpeed);
			}
			break;

		case 0xbcf819ee /* "far" */:
			{
				child->QueryFloatAttribute("range", &mFar.mRange);
				child->QueryFloatAttribute("scaledist", &mFar.mScaleDist);
				child->QueryFloatAttribute("scalespeed", &mFar.mScaleSpeed);
			}
			break;
		}
	}
	return true;
}


Aimer::Aimer(const AimerTemplate &aTemplate, unsigned int aId)
: Controller(aId)
, mTarget(0)
, mOffset(0, 0)
, mDelay(aTemplate.mTarget.mPeriod * aId / UINT_MAX)
, mWanderSidePhase(Random::Float() * 2.0f * float(M_PI))
, mWanderFrontPhase(Random::Float() * 2.0f * float(M_PI))
, mWanderTurnPhase(Random::Float() * 2.0f * float(M_PI))
{
	SetAction(Action(this, &Aimer::Control));
}

Aimer::~Aimer(void)
{
}

Vector2 Aimer::Intercept(float aLeading, const Vector2 &aPosition, const Vector2 &aVelocity)
{
#if 1
	// compute quadratic formula coefficients
	float a = aVelocity.Dot(aVelocity) - aLeading * aLeading;
	float b = aPosition.Dot(aVelocity);		// divided by 2
	float c = aPosition.Dot(aPosition);

	// compute the discriminant
	float d = b * b - a * c;

	// compute the time to intersection
	if (d > 0.0f)
		b += sqrtf(d);
	float t;
	if (fabsf(a) > FLT_EPSILON)
		t = -b / a;
	else if (fabsf(b) > FLT_EPSILON)
		t = c / -b;
	else
		t = 0.0f;

	// prevent negative time
	if (t < 0.0f)
		t = 0.0f;

	// return intersection position
	return aPosition + t * aVelocity;
#else
	// extremely simple leading based on distance
	return aPosition + aVelocity * aPosition.Length() / aLeading;
#endif
}

Vector2 Aimer::TargetDir(float aLeading, const Entity *aEntity, const Entity *aTargetEntity)
{
	// direction to target
	Vector2 targetDir(aTargetEntity->GetTransform().Transform(mOffset) - aEntity->GetPosition());

	// get target lead position
	if (aLeading != 0.0f)
	{
		targetDir = Intercept(aLeading, targetDir, aTargetEntity->GetVelocity() - aEntity->GetVelocity());
	}

	// return direction
	return targetDir;
}


// wander behavior
void Aimer::Wander(float aStep, Entity *entity, const AimerTemplate &aimer)
{
	// apply side wander
	if (aimer.mWander.mSide)
	{
		mMove.x += aimer.mWander.mSide * sinf(mWanderSidePhase);
		mWanderSidePhase += Random::Float() * aimer.mWander.mSideRate * 2.0f * float(M_PI) * sim_step;
		if (mWanderSidePhase > 2.0f * float(M_PI))
			mWanderSidePhase -= 2.0f * float(M_PI);
	}

	// apply front wander
	if (aimer.mWander.mFront)
	{
		mMove.y += aimer.mWander.mFront * sinf(mWanderFrontPhase);
		mWanderFrontPhase += Random::Float() * aimer.mWander.mFrontRate * 2.0f * float(M_PI) * sim_step;
		if (mWanderFrontPhase > 2.0f * float(M_PI))
			mWanderFrontPhase -= 2.0f * float(M_PI);
	}

	// apply turn wander
	if (aimer.mWander.mTurn)
	{
		mTurn += aimer.mWander.mTurn * sinf(mWanderTurnPhase);
		mWanderTurnPhase += Random::Float() * aimer.mWander.mTurnRate * 2.0f * float(M_PI) * sim_step;
		if (mWanderTurnPhase > 2.0f * float(M_PI))
			mWanderTurnPhase -= 2.0f * float(M_PI);
	}
}

// target behavior
void Aimer::Target(float aStep, Entity *entity, const AimerTemplate &aimer)
{
	// if ready to search...
	mDelay -= aStep;
	if (mDelay > 0.0f)
		return;

	// update the timer
	mDelay += aimer.mTarget.mPeriod;

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get the collision world
	b2World *world = Collidable::GetWorld();

	// get nearby shapes
	b2AABB aabb;
	const float lookRadius = aimer.mTarget.mRange;
	aabb.lowerBound.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
	aabb.upperBound.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
	b2Shape* shapes[b2_maxProxies];
	int32 count = world->Query(aabb, shapes, b2_maxProxies);

	// get team affiliation
	unsigned int aTeam = Database::team.Get(mId);

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
		if (!Collidable::CheckFilter(shapes[i]->GetFilterData(), aimer.mTarget.mFilter))
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
		if (range > aimer.mTarget.mRange)
			continue;

		// if using a cone angle or angle scale
		if (aimer.mTarget.mAngle < float(M_PI)*2.0f || aimer.mTarget.mAlign != 0.0f)
		{
			// get angle to target
			float aimAngle = -atan2f(localDir.x, localDir.y);

			// get local angle
			float localAngle = aimAngle - aimer.mTarget.mDirection;
			if (localAngle > float(M_PI))
				localAngle -= float(M_PI)*2.0f;
			else if (localAngle < -float(M_PI))
				localAngle += float(M_PI)*2.0f;

			// skip if outside angle
			if (fabsf(localAngle) > aimer.mTarget.mAngle)
				continue;

			// if using angle scale...
			if (aimer.mTarget.mAlign != 0.0f)
			{
				// apply angle scale
				range *= 1.0f + fabsf(localAngle) * aimer.mTarget.mAlign;
			}
		}

		// if the current target...
		if (targetId == mTarget)
		{
			// apply focus scale
			range /= aimer.mTarget.mFocus;
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
}

// pursue behavior
void Aimer::Pursue(float aStep, Entity *entity, const AimerTemplate &aimer, Entity *targetEntity)
{
	if (aimer.mPursue.mStrength == 0.0f)
		return;

	// direction to target
	Vector2 targetDir(TargetDir(aimer.mPursue.mLeading, entity, targetEntity));

	// save range
	float distSq = targetDir.LengthSq();

	// normalize direction
	targetDir *= InvSqrt(distSq);

	// move towards target
	mMove += aimer.mPursue.mStrength * targetDir;
}

// aim behavior
void Aimer::Aim(float aStep, Entity *entity, const AimerTemplate &aimer, Entity *targetEntity)
{
	if (aimer.mAim.mStrength == 0.0f && aimer.mFireCount == 0)
		return;

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// direction to target
	Vector2 targetDir(TargetDir(aimer.mAim.mLeading, entity, targetEntity));

	// save range
	float distSq = targetDir.LengthSq();

	// normalize direction
	targetDir *= InvSqrt(distSq);

	// local direction
	Vector2 localDir = transform.Unrotate(targetDir);

	// angle to target
	float aimAngle = -atan2f(localDir.x, localDir.y);

	// if aiming...
	if (aimer.mAim.mStrength != 0)
	{
		// turn towards target direction
		const ShipTemplate &ship = Database::shiptemplate.Get(mId);	// <-- hack!
		if (ship.mMaxOmega != 0.0f)
		{
			mTurn += aimer.mAim.mStrength * Clamp(aimAngle / (ship.mMaxOmega * aStep), -1.0f, 1.0f);
		}
	}

	// for each fire channel
	memset(mFire, 0, sizeof(mFire));
	for (int i = 0; i < aimer.mFireCount; ++i)
	{
		// if not set to fire and target is in range...
		if (!mFire[aimer.mFire[i].mChannel] &&
			distSq <= aimer.mFire[i].mRange * aimer.mFire[i].mRange)
		{
			float localAngle = aimAngle - aimer.mFire[i].mDirection;
			if (localAngle > float(M_PI))
				localAngle -= float(M_PI)*2.0f;
			else if (localAngle < -float(M_PI))
				localAngle += float(M_PI)*2.0f;
			if (fabsf(localAngle) <= aimer.mFire[i].mAngle)
				mFire[aimer.mFire[i].mChannel] = sqrtf(distSq) / aimer.mFire[i].mRange; // encode range into fire (HACK)
		}
	}
}

// evade behavior
void Aimer::Evade(float aStep, Entity *entity, const AimerTemplate &aimer, Entity *targetEntity)
{
	if (aimer.mEvade.mStrength == 0.0f)
		return;

	// target entity transform
	const Transform2 &targetTransform = targetEntity->GetTransform();

	// evade target's front vector
	Vector2 local(targetTransform.Untransform(entity->GetPosition()));
	if (local.y > 0)
	{
		local *= InvSqrt(local.LengthSq());
		float dir = local.x > 0 ? 1.0f : -1.0f;
		mMove += aimer.mEvade.mStrength * dir * local.y * local.y * local.y * targetTransform.Rotate(Vector2(local.y, -local.x));
	}
}

// range behavior
void Aimer::Range(float aStep, Entity *entity, const AimerTemplate &aimer, Entity *targetEntity)
{
	if (aimer.mClose.mRange <= -FLT_MAX && aimer.mFar.mRange >= FLT_MAX)
		return;

	// get direction and distance to target
	Vector2 dir = targetEntity->GetPosition() - entity->GetPosition();
	float dist = dir.Length();
	dir /= dist;

	// get target relative speed
	Vector2 vel = targetEntity->GetVelocity() - entity->GetVelocity();
	float speed = vel.Dot(dir);

	// apply close-repel force
	float repel = (dist - aimer.mClose.mRange) * aimer.mClose.mScaleDist + speed * aimer.mClose.mScaleSpeed;
	if (repel < 0.0f)
	{
		mMove += dir * repel;
	}

	// apply far-attract force
	float attract = (dist - aimer.mFar.mRange) * aimer.mFar.mScaleDist + speed * aimer.mFar.mScaleSpeed;
	if (attract > 0.0f)
	{
		mMove += dir * attract;
	}
}

// edge behavior
void Aimer::Edge(float aStep, Entity *entity, const AimerTemplate &aimer)
{
	// get transform
	const Transform2 &transform = entity->GetTransform();

	// push away from the edges of the world (HACK)
	b2AABB edge(Collidable::GetBoundary());
	edge.lowerBound.x += 64;
	edge.upperBound.x -= 64;
	edge.lowerBound.y += 64;
	edge.upperBound.y -= 64;
	Vector2 push(0, 0);
	if (transform.p.x < edge.lowerBound.x)
		push.x += (edge.lowerBound.x - transform.p.x) / 64.0f;
	if (transform.p.x > edge.upperBound.x)
		push.x += (edge.upperBound.x - transform.p.x) / 64.0f;
	if (transform.p.y < edge.lowerBound.y)
		push.y += (edge.lowerBound.y - transform.p.y) / 64.0f;
	if (transform.p.y > edge.upperBound.y)
		push.y += (edge.upperBound.y - transform.p.y) / 64.0f;
	if (push.x || push.y)
	{
		mMove += push;
		push = transform.Unrotate(push);
		mTurn += push.x < 0 ? push.Length() : -push.Length();
	}
}


// Aimer Control
void Aimer::Control(float aStep)
{
	// get parent entity
	Entity *entity = Database::entity.Get(mId);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get aimer template
	const AimerTemplate &aimer = Database::aimertemplate.Get(mId);

	// set default controls
	mMove = transform.Rotate(Vector2(0, aimer.mDrift));
	mTurn = 0;
	memset(mFire, 0, sizeof(mFire));

	// apply wander behavior
	Wander(aStep, entity, aimer);

	// apply target behavior
	Target(aStep, entity, aimer);

	// if tracking a target...
	if (mTarget)
	{
		// get the target entity
		Entity *targetEntity = Database::entity.Get(mTarget);
		if (targetEntity)
		{
			// apply pursue behavior
			Pursue(aStep, entity, aimer, targetEntity);

			// apply aim behavior
			Aim(aStep, entity, aimer, targetEntity);

			// apply evade behavior
			Evade(aStep, entity, aimer, targetEntity);

			// apply range behavior
			Range(aStep, entity, aimer, targetEntity);
		}
	}

	// push away from the edges of the world (HACK)
	Edge(aStep, entity, aimer);

#ifdef AIMER_OBSTACLE_AVOIDANCE
	// obstacle avoidance
	if (Collidable *collidable = Database::collidable.Get(mId))
	{
		b2Body *body = collidable->GetBody();
		b2Shape *shapelist = body->GetShapeList();
		const b2FilterData &filter = shapelist->GetFilterData();

		// collision probe
		b2Segment segment;
		segment.p1 = transform.p;
		segment.p2 = transform.p + 32 * mMove;

		// perform a segment test
		float lambda = 1.0f;
		b2Vec2 normal(0, 0);
		b2Shape *shape = NULL;
		Collidable::TestSegment(segment, shapelist->GetSweepRadius() * 0.5f, mId, filter.categoryBits, filter.maskBits, lambda, normal, shape);
		if (lambda < 1.0f)
		{
			float push = 1.0f - lambda;
			mMove += push * normal;
			mTurn += transform.y.Cross(normal) > 0.0f ? push : -push;
		}
	}
#endif

	// limit move to 100%
	float moveLengthSq = mMove.LengthSq();
	if (moveLengthSq > 1.0f)
		mMove *= InvSqrt(moveLengthSq);

#ifdef AIMER_DEBUG_DRAW_CONTROLS
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex2f(transform.p.x, transform.p.y);
	glVertex2f(transform.p.x + 16 * mMove.x, transform.p.y + 16 * mMove.y);
	glEnd();
#endif

	// convert to local coordinates
	mMove = transform.Unrotate(mMove);

	// limit turn to 100%
	mTurn = Clamp(mTurn, -1.0f, 1.0f);

#ifdef AIMER_DEBUG_DRAW_CONTROLS
	glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex2f(transform.p.x, transform.p.y);
	int steps = xs_CeilToInt(mTurn * 16);
	for(int i = 0; i < steps; i++)
	{
		float angle = float(M_PI) * i * mTurn / steps;
		Vector2 dir = transform.Transform(Vector2(-16*sinf(angle), 16*cosf(angle)));
		glVertex2f(dir.x, dir.y);
	}
	glEnd();
#endif
}
