#include "StdAfx.h"
#include "Bullet.h"
#include "Explosion.h"
#include <boost/pool/pool.hpp>

// bullet attributes
const float BULLET_LIFE = 1.0f;

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
: mLife(BULLET_LIFE)
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
		Database::Delete(Simulatable::id);
		return;
	}
}

void Bullet::Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount)
{
	// create an explosion at the contact point
	b2Vec2 position(aManifold[0].points[0].position - aManifold[0].points[0].separation * aManifold[0].normal);
	const unsigned int template_id = 0x70f5d327 /* "playerbulletexplosion" */;
	const unsigned int instance_id = Database::Instantiate(template_id, 0, Vector2(position), Vector2(0, 0));

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
	// kill the bullet
	// (note: this breaks collision impulse)
	mLife = 0.0f;
#endif
}
