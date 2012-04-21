#include "StdAfx.h"
#include "Explosion.h"
#include "Entity.h"
#include "Collidable.h"
#include "Damagable.h"
#include "Cancelable.h"
#include "Team.h"
#include "ExpressionConfigure.h"

#include "Bullet.h"


#ifdef USE_POOL_ALLOCATOR
// explosion pool
static MemoryPool sPool(sizeof(Explosion));
void *Explosion::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Explosion::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<ExplosionTemplate> explosiontemplate(0xbde38dea /* "explosiontemplate" */);
	Typed<Explosion *> explosion(0x02bb1fe0 /* "explosion" */);

	namespace Loader
	{
		class ExplosionLoader
		{
		public:
			ExplosionLoader()
			{
				AddConfigure(0x02bb1fe0 /* "explosion" */, Entry(this, &ExplosionLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				ExplosionTemplate &explosion = Database::explosiontemplate.Open(aId);
				explosion.Configure(element, aId);
				Database::explosiontemplate.Close(aId);
			}
		}
		explosionloader;
	}

	namespace Initializer
	{
		class ExplosionInitializer
		{
		public:
			ExplosionInitializer()
			{
				AddActivate(0xbde38dea /* "explosiontemplate" */, Entry(this, &ExplosionInitializer::Activate));
				AddDeactivate(0xbde38dea /* "explosiontemplate" */, Entry(this, &ExplosionInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const ExplosionTemplate &explosiontemplate = Database::explosiontemplate.Get(aId);
				Explosion *explosion = new Explosion(explosiontemplate, aId);
				Database::explosion.Put(aId, explosion);
				explosion->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Explosion *explosion = Database::explosion.Get(aId))
				{
					delete explosion;
					Database::explosion.Delete(aId);
				}
			}
		}
		explosioninitializer;
	}
};


ExplosionTemplate::ExplosionTemplate(void)
: mLifeSpan(0.0f)
, mFilter(Collidable::GetDefaultFilter())
{
}

ExplosionTemplate::~ExplosionTemplate(void)
{
}


bool ExplosionTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int id)
{
	element->QueryFloatAttribute("life", &mLifeSpan);

	// backwards compatibility
	float mRadiusInner = 0.0f, mRadiusOuter = 0.0f;
	if (element->QueryFloatAttribute("radius", &mRadiusOuter) == tinyxml2::XML_SUCCESS)
		mRadiusInner = mRadiusOuter * 0.5f;
	float mDamageInner = 0.0f, mDamageOuter = 0.0f;
	if (element->QueryFloatAttribute("damage", &mDamageInner) == tinyxml2::XML_SUCCESS)
		mDamageOuter = 0.0f;

	ConfigureFilterData(mFilter, element);

	if (element->FirstChildElement())
	{
		for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
		{
			switch (Hash(child->Value()))
			{
			case 0x0dba4cb3 /* "radius" */:
				{
					const char * const names[] = { "inner", "outer", "", "" };
					const float data[] = { mRadiusInner, mRadiusOuter, 0.0f, 0.0f };
					Expression::Loader<__m128>::ConfigureRoot(child, mRadius, names, data);
				}
				break;

			case 0x59e94c40 /* "damage" */:
				{
					const char * const names[] = { "inner", "outer", "", "" };
					const float data[] = { mRadiusInner, mRadiusOuter, 0.0f, 0.0f };
					Expression::Loader<__m128>::ConfigureRoot(child, mDamage, names, data);
				}
				break;

			default:
				continue;
			}
		}
	}

	return true;
}


Explosion::Explosion(void)
: Updatable(0)
, mLife(0)
{
	SetAction(Action(this, &Explosion::Update));
}

Explosion::Explosion(const ExplosionTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mLife(aTemplate.mLifeSpan)
{
	SetAction(Action(this, &Explosion::Update));
}

Explosion::~Explosion(void)
{
}

class ExplosionQueryCallback : public b2QueryCallback
{
public:
	unsigned int mId;
	ExplosionTemplate mExplosion;
	Transform2 mTransform;
	float mCurRadius[2];
	float mCurDamage[2];

public:
	virtual bool ReportFixture(b2Fixture* fixture)
	{
		// skip unhittable fixtures
		if (fixture->IsSensor())
			return true;
		if (!Collidable::CheckFilter(mExplosion.mFilter, fixture->GetFilterData()))
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
		float range = dir.Length();
		float radius = 0.5f * fixture->GetShape()->m_radius;	//fixture->ComputeSweepRadius(localPos);

		// skip if out of range
		if (range > mCurRadius[1] + radius)
			return true;

		// apply damage falloff
		float damage;
		if (range <= mCurRadius[0] - radius)
		{
			damage = mCurDamage[0];
		}
		else
		{
			float interp = (range - mCurRadius[0] + radius) / (mCurRadius[1] + radius - mCurRadius[0] + radius);
			damage = Lerp(mCurDamage[0], mCurDamage[1], interp);
		}

		// if the recipient is damagable...
		// and not healing or the target is at max health...
		if (Damagable *damagable = Database::damagable.Get(targetId))
		{
			// limit healing
			if (damage < 0)
			{
				damage = std::max(damage, damagable->GetHealth() - Database::damagabletemplate.Get(targetId).mHealth);
			}

			// apply damage
			damagable->Damage(mId, damage);
		}

		if (damage >= 0)
		{
			// if the recipient is cancelable...
			if (Cancelable *cancelable = Database::cancelable.Get(targetId))
			{
				// apply cancel
				cancelable->Cancel(targetId, mId);
			}
		}

		return true;
	}
};

void Explosion::Update(float aStep)
{
	// get explosion template properties
	const ExplosionTemplate &explosion = Database::explosiontemplate.Get(mId);

	// set up query callback
	ExplosionQueryCallback callback;
	callback.mId = mId;
	callback.mExplosion = explosion;

	// default radius and damage
	callback.mCurRadius[0] = 0.0f;
	callback.mCurRadius[1] = 0.0f;
	callback.mCurDamage[0] = 0.0f;
	callback.mCurDamage[1] = 0.0f;

	// evaluate properties
	if (!explosion.mRadius.empty())
	{
		//int index = 0;
		//ApplyInterpolator(callback.mCurRadius, 2, mRadius[0], reinterpret_cast<const float * __restrict>(&mRadius[1]), explosion.mLifeSpan - mLife, index);
		EntityContext context(&explosion.mRadius[0], explosion.mRadius.size(), explosion.mLifeSpan - mLife, mId);
		__m128 value = Expression::Evaluate<__m128>(context);
		memcpy(callback.mCurRadius, &value, sizeof(callback.mCurRadius));
	}
	if (!explosion.mDamage.empty())
	{
		//int index = 0;
		//ApplyInterpolator(callback.mCurDamage, 2, mDamage[0], reinterpret_cast<const float * __restrict>(&mDamage[1]), explosion.mLifeSpan - mLife, index);
		EntityContext context(&explosion.mDamage[0], explosion.mDamage.size(), explosion.mLifeSpan - mLife, mId);
		__m128 value = Expression::Evaluate<__m128>(context);
		memcpy(callback.mCurDamage, &value, sizeof(callback.mCurDamage));
	}

	// if applying damage...
	if ((callback.mCurDamage[0] != 0.0f) || (callback.mCurDamage[1] != 0.0f))
	{
		// if applying damage over time...
		if (explosion.mLifeSpan > 0.0f)
		{
			// scale by time step
			callback.mCurDamage[0] *= aStep;
			callback.mCurDamage[1] *= aStep;
		}

		// get parent entity
		Entity *entity = Database::entity.Get(mId);

		// world-to-local transform
		callback.mTransform = entity->GetTransform().Inverse();

		// get the collision world
		b2World *world = Collidable::GetWorld();

		// get nearby fixtures
		b2AABB aabb;
		const float lookRadius = callback.mCurRadius[1];
		aabb.lowerBound.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
		aabb.upperBound.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
		world->QueryAABB(&callback, aabb);
	}

	// advance life timer
	mLife -= aStep;

	// if expired...
	if (mLife <= 0)
	{
		// deactivate
		Deactivate();
	}
}
