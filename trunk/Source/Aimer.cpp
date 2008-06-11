#include "StdAfx.h"
#include "Aimer.h"
#include "Collidable.h"
#include "Entity.h"
#include "Link.h"
#include "Weapon.h"
#include "Damagable.h"
#include "Team.h"


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
, mCategoryBits(0xFFFF)
, mMaskBits(0xFFFF)
, mLeading(0.0f)
, mEvade(0.0f)
, mClose(0.0f)
, mFar(FLT_MAX)
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
	element->QueryFloatAttribute("leading", &mLeading);
	element->QueryFloatAttribute("evade", &mEvade);
	element->QueryFloatAttribute("close", &mClose);
	element->QueryFloatAttribute("far", &mFar);
	return true;
}


Aimer::Aimer(const AimerTemplate &aTemplate, unsigned int aId)
: Controller(aId)
, mTarget(0)
, mOffset(0, 0)
, mDelay(aTemplate.mPeriod * aId / UINT_MAX)
{
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
	// clear controls
	mMove.x = 0;
	mMove.y = 0;
	mAim.x = 0;
	mAim.y = 0;
	memset(mFire, 0, sizeof(mFire));

	// get parent entity
	Entity *entity = Database::entity.Get(mId);

	// get aimer template
	const AimerTemplate &aimer = Database::aimertemplate.Get(mId);

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
		const int32 maxCount = 256;
		b2Shape* shapes[maxCount];
		int32 count = world->Query(aabb, shapes, maxCount);

		// get team affiliation
		unsigned int aTeam = Database::team.Get(mId);

		// no target yet
		unsigned int bestTargetId = 0;
		Vector2 bestTargetPos(0, 0);
		float bestRange = FLT_MAX;

		// world-to-local transform
		Matrix2 transform(entity->GetTransform().Inverse());

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
			Vector2 dir(transform.Transform(shapePos));
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

	// do nothing if no target
	if (!mTarget)
		return;

	// get the target entity
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (!targetEntity)
		return;

	// target entity transform
	Matrix2 targetTransform(targetEntity->GetTransform());

	// aim at target lead position
	Vector2 targetPos = targetTransform.Transform(mOffset);
	if (aimer.mLeading != 0.0f)
	{
		mAim = LeadTarget(aimer.mLeading,
			targetPos - entity->GetPosition(),
			targetEntity->GetVelocity() - entity->GetVelocity()
			);
	}
	else
	{
		mAim = targetPos - entity->GetPosition();
	}

	// save range
	float distSq = mAim.LengthSq();

	// normalize aim
	mAim *= InvSqrt(distSq);

	// move in aim direction
	mMove = mAim;

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
		Vector2 dir = targetEntity->GetPosition() - entity->GetPosition();
		float dist = dir.Length();
		dir /= dist;
		if (dist < aimer.mClose)
		{
			mMove += (dist - aimer.mClose) / 16.0f * dir;
		}
		if (dist > aimer.mFar)
		{
			mMove += (dist - aimer.mFar) / 64.0f * dir;
		}
	}

	mMove *= InvSqrt(mMove.LengthSq());

	// get front vector
	Vector2 front(entity->GetTransform().y);

	// for each fire channel
	for (int i = 0; i < Controller::FIRE_CHANNELS; ++i)
	{
		// fire if lined up and within attack range
		mFire[i] = 
			(distSq < aimer.mAttack[i] * aimer.mAttack[i]) &&
			(front.Dot(mAim) > aimer.mAngle[i]);
	}
}
