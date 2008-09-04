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


AimerTemplate::AimerTemplate(void)
: mPeriod(1.0f)
, mRange(0.0f)
, mFocus(1.0f)
, mCategoryBits(0x0001)
, mMaskBits(0xFFFF)
, mDrift(0.0f)
, mWanderSide(0.0f)
, mWanderSideRate(0.0f)
, mWanderFront(0.0f)
, mWanderFrontRate(0.0f)
, mWanderTurn(0.0f)
, mWanderTurnRate(0.0f)
, mAim(1.0f)
, mLeading(0.0f)
, mPursue(1.0f)
, mEvade(0.0f)
, mClose(0.0f)
, mCloseDistScale(1.0f/16.0f)
, mCloseSpeedScale(0.0f)
, mFar(FLT_MAX)
, mFarDistScale(1.0f/64.0f)
, mFarSpeedScale(0.0f)
{
	for (int i = 0; i < Controller::FIRE_CHANNELS; ++i)
	{
		mAttack[i] = 0.0f;
		mAngle[i] = 0.95f;
	}
}

AimerTemplate::~AimerTemplate(void)
{
}

bool AimerTemplate::Configure(const TiXmlElement *element)
{
	// targeting
	element->QueryFloatAttribute("period", &mPeriod);
	element->QueryFloatAttribute("range", &mRange);
	element->QueryFloatAttribute("focus", &mFocus);

	int category = 0;
	if (element->QueryIntAttribute("category", &category) == TIXML_SUCCESS)
		mCategoryBits = (category >= 0) ? (1<<category) : 0;

	char buf[16];
	for (int i = 0; i < 16; i++)
	{
		sprintf(buf, "bit%d", i);
		int bit = 0;
		if (element->QueryIntAttribute(buf, &bit) == TIXML_SUCCESS)
		{
			if (bit)
				mMaskBits |= (1 << i);
			else
				mMaskBits &= ~(1 << i);
		}
	}

	// attack
	for (int i = 0; i < Controller::FIRE_CHANNELS; ++i)
	{
		char label[16];

		element->QueryFloatAttribute("attack", &mAttack[i]);
		sprintf(label, "attack%d", i+1);
		element->QueryFloatAttribute(label, &mAttack[i]);

		if (element->QueryFloatAttribute("angle", &mAngle[i]) == TIXML_SUCCESS)
			mAngle[i] = cosf(mAngle[i] * float(M_PI) / 180.0f);
		sprintf(label, "angle%d", i+1);
		if (element->QueryFloatAttribute(label, &mAngle[i]) == TIXML_SUCCESS)
			mAngle[i] = cosf(mAngle[i] * float(M_PI) / 180.0f);
	}

	// aiming
	element->QueryFloatAttribute("drift", &mDrift);
	element->QueryFloatAttribute("wanderside", &mWanderSide);
	element->QueryFloatAttribute("wandersiderate", &mWanderSideRate);
	element->QueryFloatAttribute("wanderfront", &mWanderFront);
	element->QueryFloatAttribute("wanderfrontrate", &mWanderFrontRate);
	element->QueryFloatAttribute("wanderturn", &mWanderTurn);
	element->QueryFloatAttribute("wanderturnrate", &mWanderTurnRate);
	element->QueryFloatAttribute("aim", &mAim);
	element->QueryFloatAttribute("leading", &mLeading);
	element->QueryFloatAttribute("pursue", &mPursue);
	element->QueryFloatAttribute("evade", &mEvade);
	element->QueryFloatAttribute("close", &mClose);
	element->QueryFloatAttribute("closedistscale", &mCloseDistScale);
	element->QueryFloatAttribute("closespeedscale", &mCloseSpeedScale);
	element->QueryFloatAttribute("far", &mFar);
	element->QueryFloatAttribute("fardistscale", &mFarDistScale);
	element->QueryFloatAttribute("farspeedscale", &mFarSpeedScale);
	return true;
}


Aimer::Aimer(const AimerTemplate &aTemplate, unsigned int aId)
: Controller(aId)
, mTarget(0)
, mOffset(0, 0)
, mDelay(aTemplate.mPeriod * aId / UINT_MAX)
, mWanderSidePhase(RandFloat() * 2.0f * float(M_PI))
, mWanderFrontPhase(RandFloat() * 2.0f * float(M_PI))
, mWanderTurnPhase(RandFloat() * 2.0f * float(M_PI))
{
	SetAction(Action(this, &Aimer::Control));
}

Aimer::~Aimer(void)
{
}

Vector2 Aimer::LeadTarget(float bulletSpeed, const Vector2 &targetPosition, const Vector2 &targetVelocity)
{
#if 1
	// compute quadratic formula coefficients
	float a = targetVelocity.Dot(targetVelocity) - bulletSpeed * bulletSpeed;
	float b = targetPosition.Dot(targetVelocity);		// divided by 2
	float c = targetPosition.Dot(targetPosition);

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
	return targetPosition + t * targetVelocity;
#else
	// extremely simple leading based on distance
	return targetPosition + targetVelocity * targetPosition.Length() / bulletSpeed;
#endif
}

// Aimer Control
void Aimer::Control(float aStep)
{
	// get parent entity
	Entity *entity = Database::entity.Get(mId);

	// get front vector
	const Matrix2 transform(entity->GetTransform());

	// get aimer template
	const AimerTemplate &aimer = Database::aimertemplate.Get(mId);

	// set default controls
	mMove = aimer.mDrift * transform.y;
	mTurn = 0;
	memset(mFire, 0, sizeof(mFire));

	// apply wander
	if (aimer.mWanderSide)
	{
		mMove.x += aimer.mWanderSide * sinf(mWanderSidePhase);
		mWanderSidePhase += RandFloat() * aimer.mWanderSideRate * 2.0f * float(M_PI) * sim_step;
		if (mWanderSidePhase > 2.0f * float(M_PI))
			mWanderSidePhase -= 2.0f * float(M_PI);
	}
	if (aimer.mWanderFront)
	{
		mMove.y += aimer.mWanderFront * sinf(mWanderFrontPhase);
		mWanderFrontPhase += RandFloat() * aimer.mWanderFrontRate * 2.0f * float(M_PI) * sim_step;
		if (mWanderFrontPhase > 2.0f * float(M_PI))
			mWanderFrontPhase -= 2.0f * float(M_PI);
	}
	if (aimer.mWanderTurn)
	{
		mTurn += aimer.mWanderTurn * sinf(mWanderTurnPhase);
		mWanderTurnPhase += RandFloat() * aimer.mWanderTurnRate * 2.0f * float(M_PI) * sim_step;
		if (mWanderTurnPhase > 2.0f * float(M_PI))
			mWanderTurnPhase -= 2.0f * float(M_PI);
	}

	// if ready to search...
	mDelay -= aStep;
	if (mDelay <= 0.0f)
	{
		// update the timer
		mDelay += aimer.mPeriod;

		// get the collision world
		b2World *world = Collidable::GetWorld();

		// get nearby shapes
		b2AABB aabb;
		const float lookRadius = aimer.mRange;
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
			if ((shapes[i]->GetFilterData().maskBits & aimer.mCategoryBits) == 0)
				continue;
			if ((shapes[i]->GetFilterData().categoryBits & aimer.mMaskBits) == 0)
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

			// get range
			Vector2 dir(transform.Untransform(shapePos));
			float range = dir.Length() - 0.5f * shapes[i]->GetSweepRadius();

			// skip if out of range
			if (range > aimer.mRange)
				continue;

			// if not the current target...
			if (targetId != mTarget)
			{
				// bias range
				range *= aimer.mFocus;
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

	// if tracking a target...
	if (mTarget)
	{
		// get the target entity
		Entity *targetEntity = Database::entity.Get(mTarget);
		if (targetEntity)
		{
			// target entity transform
			Matrix2 targetTransform(targetEntity->GetTransform());

			// direction to target
			Vector2 targetDir;

			// get target lead position
			Vector2 targetPos = targetTransform.Transform(mOffset);
			if (aimer.mLeading != 0.0f)
			{
				targetDir = LeadTarget(aimer.mLeading,
					targetPos - entity->GetPosition(),
					targetEntity->GetVelocity() - entity->GetVelocity()
					);
			}
			else
			{
				targetDir = targetPos - entity->GetPosition();
			}

			// save range
			float distSq = targetDir.LengthSq();

			// normalize direction
			targetDir *= InvSqrt(distSq);

			// move towards target
			mMove += aimer.mPursue * targetDir;

			// local target direction
			Vector2 localDir = transform.Unrotate(targetDir);

			// if aiming
			if (aimer.mAim)
			{
				// turn towards target direction
				const ShipTemplate &ship = Database::shiptemplate.Get(mId);	// <-- hack!
				if (ship.mMaxOmega != 0.0f)
				{
					float aim_angle = -atan2f(localDir.x, localDir.y);
					mTurn += aimer.mAim * std::min(std::max(aim_angle / (ship.mMaxOmega * aStep), -1.0f), 1.0f);
				}
			}

			// if evading...
			if (aimer.mEvade)
			{
				// evade target's front vector
				Vector2 local(targetTransform.Untransform(entity->GetPosition()));
				if (local.y > 0)
				{
					local *= InvSqrt(local.LengthSq());
					float dir = local.x > 0 ? 1.0f : -1.0f;
					mMove += aimer.mEvade * dir * local.y * local.y * local.y * targetTransform.Rotate(Vector2(local.y, -local.x));
				}
			}

			// if checking range...
			if (aimer.mClose > 0 || aimer.mFar > FLT_MAX)
			{
				// get direction and distance to target
				Vector2 dir = targetEntity->GetPosition() - entity->GetPosition();
				float dist = dir.Length();
				dir /= dist;

				// get target relative speed
				Vector2 vel = targetEntity->GetVelocity() - entity->GetVelocity();
				float speed = vel.Dot(dir);

				// apply close-repel force
				float repel = (dist - aimer.mClose) * aimer.mCloseDistScale + speed * aimer.mCloseSpeedScale;
				if (repel < 0.0f)
				{
					mMove += dir * repel;
				}

				// apply far-attract force
				float attract = (dist - aimer.mFar) * aimer.mFarDistScale + speed * aimer.mFarSpeedScale;
				if (attract > 0.0f)
				{
					mMove += dir * attract;
				}
			}

			// for each fire channel
			for (int i = 0; i < Controller::FIRE_CHANNELS; ++i)
			{
				// fire if lined up and within attack range
				mFire[i] = 
					(distSq < aimer.mAttack[i] * aimer.mAttack[i]) &&
					(localDir.y > aimer.mAngle[i]);
			}
		}
	}

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
		mTurn += transform.y.Cross(push) > 0 ? push.Length() : -push.Length();
	}

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
	mTurn = std::min(std::max(mTurn, -1.0f), 1.0f);

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
