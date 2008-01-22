#include "StdAfx.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Damagable.h"
#include "Link.h"

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
	if (Hash(element->Value()) != 0xe894a379 /* "bullet" */)
		return false;

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
	Database::Typed<Collidable::Listener> &listeners = Database::collidablelistener.Open(id);
	Collidable::Listener &listener = listeners.Open(Database::Key(this));
	listener.bind(this, &Bullet::Collide);
	listeners.Close(Database::Key(this));
	Database::collidablelistener.Close(id);
}

Bullet::~Bullet(void)
{
	Database::Typed<Collidable::Listener> &listeners = Database::collidablelistener.Open(id);
	listeners.Delete(Database::Key(this));
	Database::collidablelistener.Close(id);
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
				Database::Instantiate(bullet.mSpawnOnExpire, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
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

void Bullet::Collide(unsigned int aHitId, float aTime, b2Manifold aManifold[], int aCount)
{
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
			// if the recipient is damagable and needs health...
			Damagable *damagable = Database::damagable.Get(aHitId);
			if (damagable && (damagable->GetHealth() < Database::damagabletemplate.Get(aHitId).mHealth))
			{
				// apply healing value
				damagable->Damage(id, bullet.mDamage);
				destroy = true;
			}
			else
			{
				// if the recipient's owner is damagable and needs health...
				unsigned int aHitOwnerId = Database::owner.Get(aHitId);
				Damagable *damagable = Database::damagable.Get(aHitOwnerId);
				if (damagable && (damagable->GetHealth() < Database::damagabletemplate.Get(aHitOwnerId).mHealth))
				{
					// apply healing value
					damagable->Damage(id, std::max(bullet.mDamage, damagable->GetHealth() - Database::damagabletemplate.Get(aHitOwnerId).mHealth));
					destroy = true;
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
		b2Vec2 position(aManifold[0].points[0].position - aManifold[0].points[0].separation * aManifold[0].normal);

		// spawn the template
		unsigned int spawnId = Database::Instantiate(bullet.mSpawnOnImpact, 0, Vector2(position), Vector2(0, 0), 0);

		// set fractional turn
		if (Renderable *renderable = Database::renderable.Get(spawnId))
			renderable->SetFraction(aTime);
	}

	// if destroying the bullet...
	if (destroy)
	{
		// if spawning on death...
		if (bullet.mSpawnOnDeath)
		{
#ifdef USE_CHANGE_DYNAMIC_TYPE
			// change dynamic type
			Database::Deactivate(id);
			Database::parent.Put(id, bullet.mSpawnOnDeath);
			Database::Activate(id);
#else
			// get the entity
			Entity *entity = Database::entity.Get(id);
			if (entity)
			{
				// instantiate the template
				Database::Instantiate(id, bullet.mSpawnOnDeath, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
			}
#endif
		}
#ifdef USE_CHANGE_DYNAMIC_TYPE
		else
#endif
		{
			// kill the bullet
			// (note: this breaks collision impulse)
			Database::Delete(id);
		}
	}
#endif
}
