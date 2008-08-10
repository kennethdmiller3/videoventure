#include "StdAfx.h"
#include "Explosion.h"
#include "Collidable.h"
#include "Damagable.h"
#include "Team.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// explosion pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Explosion));
void *Explosion::operator new(size_t aSize)
{
	return pool.malloc();
}
void Explosion::operator delete(void *aPtr)
{
	pool.free(aPtr);
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

			void Configure(unsigned int aId, const TiXmlElement *element)
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
: mLifeSpan(0.25f)
, mCategoryBits(0xFFFF)
, mMaskBits(0xFFFF)
, mDamage(0.0f)
, mRadius(0.0f)
, mSpawnOnExpire(0)
{
}

ExplosionTemplate::~ExplosionTemplate(void)
{
}


bool ExplosionTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	element->QueryFloatAttribute("life", &mLifeSpan);
	element->QueryFloatAttribute("damage", &mDamage);
	element->QueryFloatAttribute("radius", &mRadius);
	if (const char *spawn = element->Attribute("spawnonexpire"))
		mSpawnOnExpire = Hash(spawn);

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

	return true;
}


Explosion::Explosion(void)
: Updatable(0)
, mLife(0)
{
}

Explosion::Explosion(const ExplosionTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mLife(aTemplate.mLifeSpan)
{
	if (aTemplate.mRadius > 0.0f)
	{
		// get parent entity
		Entity *entity = Database::entity.Get(mId);

		// get the collision world
		b2World *world = Collidable::GetWorld();

		// get nearby shapes
		b2AABB aabb;
		const float lookRadius = aTemplate.mRadius;
		aabb.lowerBound.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
		aabb.upperBound.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
		b2Shape* shapes[b2_maxProxies];
		int32 count = world->Query(aabb, shapes, b2_maxProxies);

		// world-to-local transform
		Matrix2 transform(entity->GetTransform().Inverse());

		// for each shape...
		for (int32 i = 0; i < count; ++i)
		{
			// get the shape
			b2Shape* shape = shapes[i];

			// skip unhittable shapes
			if (shape->IsSensor())
				continue;
			if ((shape->GetFilterData().maskBits & aTemplate.mCategoryBits) == 0)
				continue;
			if ((shape->GetFilterData().categoryBits & aTemplate.mMaskBits) == 0)
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
			Vector2 dir(transform.Transform(Vector2(body->GetPosition())));
			float range = dir.Length() - 0.5f * shapes[i]->GetSweepRadius();

			// skip if out of range
			if (range > aTemplate.mRadius)
				continue;

			// if the recipient is damagable...
			// and not healing or the target is at max health...
			Damagable *damagable = Database::damagable.Get(targetId);
			if (damagable)
			{
				// get base damage
				float damage = aTemplate.mDamage;

				// apply damage falloff
				if (range > 0)
				{
					damage *= (1.0f - (range * range) / (aTemplate.mRadius * aTemplate.mRadius));
				}

				// limit healing
				if (damage < 0)
				{
					damage = std::max(damage, damagable->GetHealth() - Database::damagabletemplate.Get(targetId).mHealth);
				}

				// apply damage
				damagable->Damage(mId, damage);
			}
		}
	}
}

Explosion::~Explosion(void)
{
}

void Explosion::Update(float aStep)
{
	// advance life timer
	mLife -= aStep;

	// if expired...
	if (mLife <= 0)
	{
		// if spawning on expire...
		const ExplosionTemplate &explosion = Database::explosiontemplate.Get(mId);
		if (explosion.mSpawnOnExpire)
		{
#ifdef USE_CHANGE_DYNAMIC_TYPE
			// change dynamic type
			Database::Deactivate(mId);
			Database::parent.Put(mId, explosion.mSpawnOnExpire);
			Database::Activate(mId);
#else
			// get the entity
			Entity *entity = Database::entity.Get(mId);
			if (entity)
			{
				// spawn template at the entity location
				Database::Instantiate(explosion.mSpawnOnExpire, Database::owner.Get(mId), entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
			}
#endif
		}
#ifdef USE_CHANGE_DYNAMIC_TYPE
		else
#endif
		{
			// delete the entity
			Database::Delete(mId);
		}

		return;
	}
}
