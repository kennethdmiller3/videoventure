#include "StdAfx.h"
#include "Bullet.h"
#include "Entity.h"
#include "Collidable.h"
#include "Explosion.h"
#include "Damagable.h"
#include "Cancelable.h"
#include "Link.h"
#include "Team.h"
#include "Expire.h"
#include "Renderable.h"


#ifdef USE_POOL_ALLOCATOR

// bullet pool
static MemoryPool sPool(sizeof(Bullet));
void *Bullet::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Bullet::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<BulletTemplate> bullettemplate(0xa270491f /* "bullettemplate" */);
	Typed<Bullet *> bullet(0xe894a379 /* "bullet" */);

	namespace Loader
	{
		static void BulletConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			BulletTemplate &bullet = Database::bullettemplate.Open(aId);
			bullet.Configure(element);
			Database::bullettemplate.Close(aId);
		}
		Configure bulletconfigure(0xe894a379 /* "bullet" */, BulletConfigure);
	}

	namespace Initializer
	{
		void BulletActivate(unsigned int aId)
		{
			const BulletTemplate &bullettemplate = Database::bullettemplate.Get(aId);
			Bullet *bullet = new Bullet(bullettemplate, aId);
			Database::bullet.Put(aId, bullet);
		}
		Activate bulletactivate(0xa270491f /* "bullettemplate" */, BulletActivate);

		void BulletDeactivate(unsigned int aId)
		{
			if (Bullet *bullet = Database::bullet.Get(aId))
			{
				delete bullet;
				Database::bullet.Delete(aId);
			}
		}
		Deactivate bulletdeactivate(0xa270491f /* "bullettemplate" */, BulletDeactivate);
	}
}


BulletTemplate::BulletTemplate(void)
: mDamage(0), mRicochet(false), mSpawnOnImpact(0), mSpawnOnDeath(0), mSwitchOnDeath(0)
{
}

BulletTemplate::~BulletTemplate(void)
{
}

bool BulletTemplate::Configure(const tinyxml2::XMLElement *element)
{
	element->QueryFloatAttribute("damage", &mDamage);
	element->QueryBoolAttribute("ricochet", &mRicochet);
	if (const char *spawn = element->Attribute("spawnonimpact"))
		mSpawnOnImpact = Hash(spawn);
	if (const char *spawn = element->Attribute("spawnondeath"))
		mSpawnOnDeath = Hash(spawn);
	if (const char *spawn = element->Attribute("switchondeath"))
		mSwitchOnDeath = Hash(spawn);
	return true;
}


Bullet::Bullet(void)
: mId(0), mDestroy(false)
{
}

Bullet::Bullet(const BulletTemplate &aTemplate, unsigned int aId)
: mId(aId), mDestroy(false)
{
	Collidable::ContactSignal &signal = Database::collidablecontactadd.Open(mId);
	signal.Connect(this, &Bullet::Collide);
	Database::collidablecontactadd.Close(mId);
}

Bullet::~Bullet(void)
{
	Collidable::ContactSignal &signal = Database::collidablecontactadd.Open(mId);
	signal.Disconnect(this, &Bullet::Collide);
	Database::collidablecontactadd.Close(mId);
}

void Bullet::Kill(float aFraction)
{
	const BulletTemplate &bullet = Database::bullettemplate.Get(mId);

	// if spawning on death...
	if (bullet.mSpawnOnDeath)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// spawn template at entity location
			unsigned int spawnId = Database::Instantiate(bullet.mSpawnOnDeath, Database::owner.Get(mId), mId,
				entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
			if (Renderable *renderable = Database::renderable.Get(spawnId))
				renderable->SetFraction(aFraction);
		}
	}

	// if switching on death...
	if (bullet.mSwitchOnDeath)
	{
		// change dynamic type
		unsigned int aId = mId;
		Database::Switch(aId, bullet.mSwitchOnDeath);
		if (Renderable *renderable = Database::renderable.Get(aId))
			renderable->SetFraction(aFraction);
	}
	else
	{
		// delete the entity
		Database::Delete(mId);
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
		SetAction(Action(this, &BulletKillUpdate::Update));
		Activate();
	}

	void Update(float aStep)
	{
		if (Bullet *bullet = Database::bullet.Get(mId))
			bullet->Kill(mTime);
		Deactivate();
		delete this;
	}
};

#ifdef USE_POOL_ALLOCATOR
// kill update pool
static MemoryPool sKillPool(sizeof(BulletKillUpdate));

void *BulletKillUpdate::operator new(size_t aSize)
{
	return sKillPool.Alloc();
}
void BulletKillUpdate::operator delete(void *aPtr)
{
	sKillPool.Free(aPtr);
}
#endif


void Bullet::Collide(unsigned int aId, unsigned int aHitId, float aFraction, const b2Contact &aContact)
{
	// do nothing if destroyed...
	if (mDestroy)
		return;
	assert(mId == aId);

	const BulletTemplate &bullet = Database::bullettemplate.Get(mId);

	// get team affiliation
	unsigned int aTeam = Database::team.Get(mId);
	unsigned int aHitTeam = Database::team.Get(aHitId);

	// if the bullet applies damage...
	mDestroy = !bullet.mRicochet;
	if (bullet.mDamage >= 0)
	{
		// if not hitting a teammate...
		if (!aTeam || !aHitTeam || aTeam != aHitTeam)
		{
			// if the recipient is damagable...
			if (Damagable *damagable = Database::damagable.Get(aHitId))
			{
				// apply damage value
#ifdef DEBUG_BULLET_APPLY_DAMAGE
				DebugPrint("bullet=\"%s\" owner=\"%s\" hit=\"%s\" damage=%f\n",
					Database::name.Get(mId).c_str(), 
					Database::name.Get(Database::owner.Get(mId)).c_str(),
					Database::name.Get(aHitId).c_str(),
					bullet.mDamage
					);
#endif
				damagable->Damage(mId, bullet.mDamage);
				mDestroy = true;
			}

			// if the recipient is cancelable...
			if (Cancelable *cancelable = Database::cancelable.Get(aHitId))
			{
				// apply cancel
				cancelable->Cancel(aHitId, mId);
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
#ifdef DEBUG_BULLET_APPLY_DAMAGE
						DebugPrint("bullet=\"%s\" owner=\"%s\" hit=\"%s\" damage=%f\n",
							Database::name.Get(mId).c_str(), 
							Database::name.Get(Database::owner.Get(mId)).c_str(),
							Database::name.Get(aHitId).c_str(),
							bullet.mDamage
							);
#endif
						damagable->Damage(mId, std::max(bullet.mDamage, curhealth - maxhealth));
						mDestroy = true;
						break;
					}
				}
			}
		}
	}

#ifdef BULLET_COLLISION_BOUNCE
	// reorient to new direction
	if (b2Body *body = Database::collidablebody.Get(mId);
	{
		const b2Vec2 velocity = body->GetLinearVelocity();
		float angle = -atan2f(velocity.x, velocity.y);
		body->SetOriginPosition(body->GetOriginPosition(), angle);
		Entity *entity = Database::entity.Get(mId);
		entity->Step();
		entity->SetTransform(Matrix2(body->GetRotationMatrix(), body->GetOriginPosition()));
		entity->SetVelocity(Vector2(body->GetLinearVelocity()));
	}
#else
	// if spawning on impact...
	if (bullet.mSpawnOnImpact)
	{
		b2WorldManifold worldManifold;
		aContact.GetWorldManifold(&worldManifold);

		// estimate the point of impact
		b2Vec2 position(worldManifold.points[0]);

		// spawn the template
		unsigned int spawnId = Database::Instantiate(bullet.mSpawnOnImpact, Database::owner.Get(mId), mId, 0, Vector2(position), Vector2(0, 0), 0);

		// set fractional turn
		if (Renderable *renderable = Database::renderable.Get(spawnId))
			renderable->SetFraction(aFraction);
	}
#endif

	if (mDestroy)
	{
		new BulletKillUpdate(mId, aFraction);
	}
}
