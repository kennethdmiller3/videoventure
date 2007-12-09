#include "StdAfx.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Damagable.h"
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


namespace Database
{
	Typed<BulletTemplate> bullettemplate("bullettemplate");
	Typed<Bullet *> bullet("bullet");
}


BulletTemplate::BulletTemplate(void)
: mLife(FLT_MAX), mDamage(0), mRicochet(false), mSpawnOnDeath(0), mSpawnOnExpire(0)
{
}

BulletTemplate::~BulletTemplate(void)
{
}

bool BulletTemplate::Configure(TiXmlElement *element)
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
	Collidable *collidable = Database::collidable.Get(Simulatable::id);
	if (collidable)
		collidable->listeners[Simulatable::id] = this;
}

Bullet::~Bullet(void)
{
	Collidable *collidable = Database::collidable.Get(Simulatable::id);
	if (collidable)
		collidable->listeners.erase(Simulatable::id);
}

void Bullet::Simulate(float aStep)
{
	// count down life
	mLife -= aStep;
	if (mLife <= 0)
	{
		Entity *entity = Database::entity.Get(id);
		if (entity)
		{
			const BulletTemplate &bullet = Database::bullettemplate.Get(Simulatable::id);
			if (bullet.mSpawnOnExpire)
			{
				Database::Instantiate(bullet.mSpawnOnExpire, entity->GetAngle(), entity->GetPosition(), Vector2(0, 0));
			}
		}
		Database::Delete(Simulatable::id);
		return;
	}
}

void Bullet::Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount)
{
	const BulletTemplate &bullet = Database::bullettemplate.Get(Simulatable::id);

	// if the recipient is damagable...
	// and not healing or the target is at max health...
	unsigned int hitId = aRecipient.GetId();
	Damagable *damagable = Database::damagable.Get(hitId);
	if (damagable && (bullet.mDamage >= 0 || damagable->GetHealth() < Database::damagabletemplate.Get(hitId).mHealth))
	{
		// apply damage value
		damagable->Damage(Simulatable::id, bullet.mDamage);
	}

	// else if set to ricochet...
	else if (bullet.mRicochet)
	{
		// collide normally
		return;
	}


#ifdef BULLET_COLLISION_BOUNCE
	// reorient to new direction
	Collidable *collidable = Database::collidable.Get(Simulatable::id);
	if (collidable)
	{
		b2Body *body = collidable->GetBody();
		const b2Vec2 velocity = body->GetLinearVelocity();
		float angle = -atan2f(velocity.x, velocity.y);
		body->SetOriginPosition(body->GetOriginPosition(), angle);
		Entity *entity = Database::entity.Get(Simulatable::id);
		entity->Step();
		entity->SetTransform(Matrix2(body->GetRotationMatrix(), body->GetOriginPosition()));
		entity->SetVelocity(Vector2(body->GetLinearVelocity()));
	}
#else
	// estimate the point of impact
	b2Vec2 position(aManifold[0].points[0].position - aManifold[0].points[0].separation * aManifold[0].normal);

	// if spawning on death...
	if (bullet.mSpawnOnDeath)
	{
		// spawn the template
		Database::Instantiate(bullet.mSpawnOnDeath, 0, Vector2(position), Vector2(0, 0));
	}

	// kill the bullet
	// (note: this breaks collision impulse)
	Database::Delete(Simulatable::id);
#endif
}
