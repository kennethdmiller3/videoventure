#include "StdAfx.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Damagable.h"
#include "Link.h"
#include "Team.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>


// bullet pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Bullet));
void *Bullet::operator new(size_t aSize)
{
	return pool.malloc();
}
void Bullet::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<BulletTemplate> bullettemplate(0xa270491f /* "bullettemplate" */);
	Typed<Bullet *> bullet(0xe894a379 /* "bullet" */);

	namespace Loader
	{
		class BulletLoader
		{
		public:
			BulletLoader()
			{
				AddConfigure(0xe894a379 /* "bullet" */, Entry(this, &BulletLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				BulletTemplate &bullet = Database::bullettemplate.Open(aId);
				bullet.Configure(element);
				Database::bullettemplate.Close(aId);
			}
		}
		bulletloader;
	}

	namespace Initializer
	{
		class BulletInitializer
		{
		public:
			BulletInitializer()
			{
				AddActivate(0xa270491f /* "bullettemplate" */, Entry(this, &BulletInitializer::Activate));
				AddDeactivate(0xa270491f /* "bullettemplate" */, Entry(this, &BulletInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const BulletTemplate &bullettemplate = Database::bullettemplate.Get(aId);
				Bullet *bullet = new Bullet(bullettemplate, aId);
				Database::bullet.Put(aId, bullet);
				bullet->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Bullet *bullet = Database::bullet.Get(aId))
				{
					delete bullet;
					Database::bullet.Delete(aId);
				}
			}
		}
		bulletinitializer;
	}
}


BulletTemplate::BulletTemplate(void)
: mLife(FLT_MAX), mDamage(0), mRicochet(false), mSpawnOnDeath(0), mSpawnOnExpire(0)
{
}

BulletTemplate::~BulletTemplate(void)
{
}

bool BulletTemplate::Configure(const TiXmlElement *element)
{
	element->QueryFloatAttribute("life", &mLife);
	element->QueryFloatAttribute("damage", &mDamage);
	int ricochet = mRicochet;
	element->QueryIntAttribute("ricochet", &ricochet);
	mRicochet = ricochet != 0;
	if (const char *spawn = element->Attribute("spawnonexpire"))
		mSpawnOnExpire = Hash(spawn);
	if (const char *spawn = element->Attribute("spawnondeath"))
		mSpawnOnDeath = Hash(spawn);
	if (const char *spawn = element->Attribute("spawnonimpact"))
		mSpawnOnImpact = Hash(spawn);
	return true;
}


Bullet::Bullet(void)
: Simulatable(0)
, mLife(0.0f)
{
}

Bullet::Bullet(const BulletTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
, mLife(aTemplate.mLife)
{
	Database::Typed<Collidable::Listener> &listeners = Database::collidablecontactadd.Open(id);
	Collidable::Listener &listener = listeners.Open(Database::Key(this));
	listener.bind(this, &Bullet::Collide);
	listeners.Close(Database::Key(this));
	Database::collidablecontactadd.Close(id);
}

Bullet::~Bullet(void)
{
	Database::Typed<Collidable::Listener> &listeners = Database::collidablecontactadd.Open(id);
	listeners.Delete(Database::Key(this));
	Database::collidablecontactadd.Close(id);
}

void Bullet::Simulate(float aStep)
{
	// count down life
	mLife -= aStep;

	// if expired...
	if (mLife <= 0)
	{
		// if spawning on expire...
		const BulletTemplate &bullet = Database::bullettemplate.Get(id);
		if (bullet.mSpawnOnExpire)
		{
#ifdef USE_CHANGE_DYNAMIC_TYPE
			// change dynamic type
			Database::Deactivate(id);
			Database::parent.Put(id, bullet.mSpawnOnExpire);
			Database::Activate(id);
#else
			// get the entity
			Entity *entity = Database::entity.Get(id);
			if (entity)
			{
				// spawn template at entity location
				Database::Instantiate(bullet.mSpawnOnExpire, Database::owner.Get(id), entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
			}
#endif
		}
#ifdef USE_CHANGE_DYNAMIC_TYPE
		else
#endif
		{
			// delete the entity
			Database::Delete(id);
		}

		return;
	}
}

void Bullet::Kill(float aFraction)
{
		// if spawning on expire...
		const BulletTemplate &bullet = Database::bullettemplate.Get(id);
		if (bullet.mSpawnOnDeath)
		{
#ifdef USE_CHANGE_DYNAMIC_TYPE
			// change dynamic type
			Database::Deactivate(id);
			Database::parent.Put(id, bullet.mSpawnOnDeath);
			Database::Activate(id);
			if (Renderable *renderable = Database::renderable.Get(id))
				renderable->SetFraction(aFraction);
#else
			// get the entity
			Entity *entity = Database::entity.Get(id);
			if (entity)
			{
				// spawn template at entity location
				unsigned int spawnId = Database::Instantiate(bullet.mSpawnOnDeath, Database::owner.Get(id), entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
				if (Renderable *renderable = Database::renderable.Get(spawnId))
					renderable->SetFraction(aFraction);
			}
#endif
		}
#ifdef USE_CHANGE_DYNAMIC_TYPE
		else
#endif
		{
			// delete the entity
			Database::Delete(id);
		}

		return;
}


class BulletKillUpdate : public Updatable
{
	float mTime;

public:
#ifdef USE_POOL_ALLOCATOR
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	BulletKillUpdate(unsigned int aId, float aFraction)
		: Updatable(aId), mTime(aFraction)
	{
		Activate();
	}

	void Update(float aStep)
	{
		if (Bullet *bullet = Database::bullet.Get(id))
			bullet->Kill(mTime);
		Deactivate();
		delete this;
	}
};

#ifdef USE_POOL_ALLOCATOR
// kill update pool
static boost::pool<boost::default_user_allocator_malloc_free> killpool(sizeof(BulletKillUpdate));

void *BulletKillUpdate::operator new(size_t aSize)
{
	return killpool.malloc();
}
void BulletKillUpdate::operator delete(void *aPtr)
{
	killpool.free(aPtr);
}
#endif


void Bullet::Collide(unsigned int aId, unsigned int aHitId, float aFraction, const b2ContactPoint &aPoint)
{
	// do nothing if expired...
	if (mLife <= 0)
		return;
	assert(id == aId);

	const BulletTemplate &bullet = Database::bullettemplate.Get(id);

	// get team affiliation
	unsigned int aTeam = Database::team.Get(id);
	unsigned int aHitTeam = Database::team.Get(aHitId);

	// if the bullet applies damage...
	bool destroy = !bullet.mRicochet;
	if (bullet.mDamage >= 0)
	{
		// if not hitting a teammate...
		if (!aTeam || !aHitTeam || aTeam != aHitTeam)
		{
			// if the recipient is damagable...
			Damagable *damagable = Database::damagable.Get(aHitId);
			if (damagable)
			{
				// apply damage value
				damagable->Damage(id, bullet.mDamage);
				destroy = true;
			}
		}
	}
	else
	{
		// if not hitting an enemy...
		if (!aTeam || !aHitTeam || aTeam == aHitTeam)
		{
			// for each candidate recipient...
			for (unsigned int aRecipientId = aHitId; aRecipientId; aRecipientId = Database::backlink.Get(aRecipientId))
			{
				// if the recipient is damagable and needs health...
				Damagable *damagable = Database::damagable.Get(aRecipientId);
				if (damagable)
				{
					float curhealth = damagable->GetHealth();
					float maxhealth = Database::damagabletemplate.Get(aRecipientId).mHealth;
					if (curhealth < maxhealth)
					{
						// apply healing value
						damagable->Damage(id, std::max(bullet.mDamage, curhealth - maxhealth));
						destroy = true;
						break;
					}
				}
			}
		}
	}

#ifdef BULLET_COLLISION_BOUNCE
	// reorient to new direction
	Collidable *collidable = Database::collidable.Get(id);
	if (collidable)
	{
		b2Body *body = collidable->GetBody();
		const b2Vec2 velocity = body->GetLinearVelocity();
		float angle = -atan2f(velocity.x, velocity.y);
		body->SetOriginPosition(body->GetOriginPosition(), angle);
		Entity *entity = Database::entity.Get(id);
		entity->Step();
		entity->SetTransform(Matrix2(body->GetRotationMatrix(), body->GetOriginPosition()));
		entity->SetVelocity(Vector2(body->GetLinearVelocity()));
	}
#else
	// if spawning on impact...
	if (bullet.mSpawnOnImpact)
	{
		// estimate the point of impact
		b2Vec2 position(aPoint.position - aPoint.separation * aPoint.normal);

		// spawn the template
		unsigned int spawnId = Database::Instantiate(bullet.mSpawnOnImpact, Database::owner.Get(id), 0, Vector2(position), Vector2(0, 0), 0);

		// set fractional turn
		if (Renderable *renderable = Database::renderable.Get(spawnId))
			renderable->SetFraction(aFraction);
	}
#endif

	if (destroy)
	{
		new BulletKillUpdate(id, aFraction);
		mLife = 0.0f;
	}
}
