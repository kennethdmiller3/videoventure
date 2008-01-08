#include "StdAfx.h"
#include "Explosion.h"
#include "Collidable.h"
#include "Damagable.h"


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
, mSpawnOnExpire(0)
{
}

ExplosionTemplate::~ExplosionTemplate(void)
{
}


bool ExplosionTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	if (Hash(element->Value()) != 0x02bb1fe0 /* "explosion" */)
		return false;

	element->QueryFloatAttribute("life", &mLifeSpan);
	element->QueryFloatAttribute("damage", &mDamage);
	element->QueryFloatAttribute("radius", &mRadius);
	if (const char *spawn = element->Attribute("spawnonexpire"))
		mSpawnOnExpire = Hash(spawn);

	return true;
}


Explosion::Explosion(void)
: Simulatable(0)
, mLife(0)
{
}

Explosion::Explosion(const ExplosionTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
, mLife(aTemplate.mLifeSpan)
{
	if (aTemplate.mRadius > 0.0f)
	{
		// get parent entity
		Entity *entity = Database::entity.Get(id);

		// get the collision world
		b2World *world = Collidable::GetWorld();

		// get nearby shapes
		b2AABB aabb;
		const float lookRadius = aTemplate.mRadius;
		aabb.minVertex.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
		aabb.maxVertex.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
		const int32 maxCount = 256;
		b2Shape* shapes[maxCount];
		int32 count = world->Query(aabb, shapes, maxCount);

		// get team affiliation
		unsigned int aTeam = Database::team.Get(id);

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

			// skip teammate
			if (targetTeam == aTeam)
				continue;

			// get range
			Vector2 dir(transform.Transform(Vector2(shapes[i]->GetPosition())));
			float range = dir.LengthSq();

			// skip if out of range
			if (range > aTemplate.mRadius * aTemplate.mRadius)
				continue;

			// if the recipient is damagable...
			// and not healing or the target is at max health...
			Damagable *damagable = Database::damagable.Get(targetId);
			if (damagable && (aTemplate.mDamage >= 0 || damagable->GetHealth() < Database::damagabletemplate.Get(targetId).mHealth))
			{
				// apply damage value
				damagable->Damage(id, aTemplate.mDamage * (1.0f - range / (aTemplate.mRadius * aTemplate.mRadius)));
			}
		}
	}
}

Explosion::~Explosion(void)
{
}

void Explosion::Simulate(float aStep)
{
	// advance life timer
	mLife -= aStep;

	// if expired...
	if (mLife <= 0)
	{
		// if spawning on expire...
		const ExplosionTemplate &explosion = Database::explosiontemplate.Get(id);
		if (explosion.mSpawnOnExpire)
		{
			// get the entity
			Entity *entity = Database::entity.Get(id);
			if (entity)
			{
				// spawn template at the entity location
				Database::Instantiate(explosion.mSpawnOnExpire, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity());
			}
		}

		// delete the entity
		Database::Delete(id);
		return;
	}
}
