#include "StdAfx.h"
#include "Aimer.h"
#include "Collidable.h"
#include "Entity.h"
#include "Link.h"
#include "Weapon.h"


namespace Database
{
	Typed<AimerTemplate> aimertemplate(0x9bde0ae7 /* "aimertemplate" */);
	Typed<Aimer *> aimer(0x2ea90881 /* "aimer" */);

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
: mRange(256.0f)
, mAttack(256.0f)
, mFocus(1.0f)
{
}

AimerTemplate::~AimerTemplate(void)
{
}

bool AimerTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x2ea90881 /* "aimer" */)
		return false;

	element->QueryFloatAttribute("range", &mRange);
	element->QueryFloatAttribute("attack", &mAttack);
	element->QueryFloatAttribute("focus", &mFocus);
	return true;
}


Aimer::Aimer(const AimerTemplate &aTemplate, unsigned int aId)
: Controller(aId)
, mTarget(0)
{
}

Aimer::~Aimer(void)
{
}

Vector2 Aimer::LeadTarget( const Vector2 &startPosition, float bulletSpeed, const Vector2 &targetPosition, const Vector2 &targetVelocity )
{
	// extremely simple leading based on distance
	Vector2 dpos = targetPosition - startPosition;
	return dpos + targetVelocity * dpos.Length() / bulletSpeed;
}

// Aimer Control
void Aimer::Control(float aStep)
{
	// clear controls
	mMove.x = 0;
	mMove.y = 0;
	mAim.x = 0;
	mAim.y = 0;
	mFire = 0;

	// get parent entity
	Entity *entity = Database::entity.Get(id);

	// get aimer template
	const AimerTemplate &aimer = Database::aimertemplate.Get(id);

	// get the collision world
	b2World *world = Collidable::GetWorld();

	// get nearby shapes
	b2AABB aabb;
	const float lookRadius = aimer.mRange;
	aabb.minVertex.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
	aabb.maxVertex.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
	const int32 maxCount = 256;
	b2Shape* shapes[maxCount];
	int32 count = world->Query(aabb, shapes, maxCount);

	// get team affiliation
	unsigned int aTeam = Database::team.Get(id);

	// no target yet
	unsigned int bestTargetId = 0;
	float bestRange = FLT_MAX;

	// world-to-local transform
	Matrix2 transform(entity->GetTransform().Inverse());

	// for each shape...
	for (int32 i = 0; i < count; ++i)
	{
		// get the parent body
		b2Body* body = shapes[i]->m_body;

		// get the collidable id
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip non-entity
		if (targetId == 0)
			continue;

		// skip self
		if (targetId == id)
			continue;

		// get team affiliation
		unsigned int targetTeam = Database::team.Get(targetId);

		// skip neutral
		if (targetTeam == 0)
			continue;

		// skip teammate
		if (targetTeam == aTeam)
			continue;

		// get range
		Vector2 dir(transform.Transform(Vector2(shapes[i]->GetPosition())));
		float range = dir.LengthSq();

		// skip if out of range
		if (range > aimer.mRange * aimer.mRange)
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
		}
	}

	// use the new target
	mTarget = bestTargetId;

	// do nothing if no target
	if (!mTarget)
		return;

	// get weapon template (if any)
	const WeaponTemplate *weapon = NULL;
	for (Database::Typed<Link *>::Iterator itor(Database::link.Find(id)); itor.IsValid(); ++itor)
	{
		if (const WeaponTemplate *w = Database::weapontemplate.Find(itor.GetValue()->GetSecondary()))
			weapon = w;
	}

	// aim at target lead position
	Entity *targetEntity = Database::entity.Get(mTarget);
	if (weapon && weapon->mVelocity.y > 0)
	{
		mAim = LeadTarget(
			entity->GetPosition(),
			weapon->mVelocity.y,
			targetEntity->GetPosition(),
			targetEntity->GetVelocity()
			);
	}
	else
	{
		mAim = targetEntity->GetPosition() - entity->GetPosition();
	}
	mAim /= (mAim.Length() + FLT_EPSILON);
	mMove = mAim;

	// fire if lined up and within attack range
	mFire = (entity->GetTransform().y.Dot(mAim) > 0.95f) && 
		entity->GetPosition().DistSq(targetEntity->GetPosition()) < aimer.mAttack * aimer.mAttack;
}
