#include "StdAfx.h"
#include "Explosion.h"
#include "Entity.h"
#include "Collidable.h"
#include "Damagable.h"
#include "Team.h"
#include "Interpolator.h"

#include "Bullet.h"


#ifdef USE_POOL_ALLOCATOR
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
	Typed<Typed<std::vector<unsigned int> > > explosionproperty(0xe6894b09 /* "explosionproperty" */);

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
: mLifeSpan(0.0f)
, mFilter(Collidable::GetDefaultFilter())
, mRadiusInner(0.0f)
, mRadiusOuter(0.0f)
, mDamageInner(0.0f)
, mDamageOuter(0.0f)
{
}

ExplosionTemplate::~ExplosionTemplate(void)
{
}


bool ExplosionTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	element->QueryFloatAttribute("life", &mLifeSpan);

	// backwards compatibility
	if (element->QueryFloatAttribute("radius", &mRadiusOuter) == TIXML_SUCCESS)
		mRadiusInner = mRadiusOuter * 0.5f;
	if (element->QueryFloatAttribute("damage", &mDamageInner) == TIXML_SUCCESS)
		mDamageOuter = 0.0f;

	ConfigureFilterData(mFilter, element);

	if (element->FirstChildElement())
	{
		Database::Typed<std::vector<unsigned int> > &properties = Database::explosionproperty.Open(id);
		for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
		{
			switch (Hash(child->Value()))
			{
			case 0x0dba4cb3 /* "radius" */:
				child->QueryFloatAttribute("inner", &mRadiusInner);
				child->QueryFloatAttribute("outer", &mRadiusOuter);
				break;

			case 0x59e94c40 /* "damage" */:
				child->QueryFloatAttribute("inner", &mDamageInner);
				child->QueryFloatAttribute("outer", &mDamageOuter);
				break;

			default:
				continue;
			}

			// if the property has keyframes...
			if (child->FirstChildElement())
			{
				// process the interpolator item
				unsigned int propId = Hash(child->Value());
				std::vector<unsigned int> &buffer = properties.Open(propId);
				const char *names[2] = { "inner", "outer" };
				const float data[2] = { 0.0f, 0.0f };
				ConfigureInterpolatorItem(child, buffer, 2, names, data);
				properties.Close(propId);
			}
		}
		Database::explosionproperty.Close(id);
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
	ExplosionTemplate mExplosion;
	unsigned int mId;
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

		// if the recipient is damagable...
		// and not healing or the target is at max health...
		if (Damagable *damagable = Database::damagable.Get(targetId))
		{
			// apply damage falloff
			float interp;
			if (range <= mCurRadius[0] - radius)
				interp = 0.0f;
			else
				interp = (range - mCurRadius[0] + radius) / (mCurRadius[1] + radius - mCurRadius[0] + radius);
			float damage = Lerp(mCurDamage[0], mCurDamage[1], interp);

			// limit healing
			if (damage < 0)
			{
				damage = std::max(damage, damagable->GetHealth() - Database::damagabletemplate.Get(targetId).mHealth);
			}

			// apply damage
			damagable->Damage(mId, damage);
		}

		// if the recipient is a bullet...
		// TO DO: handle this in a more general way--a "cancelable" component?
		else if (Bullet *bullet = Database::bullet.Get(targetId))
		{
			// cancel the bullet
			Database::Delete(targetId);
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
	callback.mExplosion = explosion;

	// default radius and damage
	callback.mCurRadius[0] = explosion.mRadiusInner;
	callback.mCurRadius[1] = explosion.mRadiusOuter;
	callback.mCurDamage[0] = explosion.mDamageInner;
	callback.mCurDamage[1] = explosion.mDamageOuter;

	// get animated properties (if any)
	if (const Database::Typed<std::vector<unsigned int> > *properties = Database::explosionproperty.Find(mId))
	{
		const std::vector<unsigned int> &radiusbuffer = properties->Get(0x0dba4cb3 /* "radius" */);
		if (!radiusbuffer.empty())
		{
			int index = 0;
			ApplyInterpolator(callback.mCurRadius, 2, radiusbuffer[0], reinterpret_cast<const float * __restrict>(&radiusbuffer[1]), explosion.mLifeSpan - mLife, index);
		}
		const std::vector<unsigned int> &damagebuffer = properties->Get(0x59e94c40 /* "damage" */);
		if (!damagebuffer.empty())
		{
			int index = 0;
			ApplyInterpolator(callback.mCurDamage, 2, damagebuffer[0], reinterpret_cast<const float * __restrict>(&damagebuffer[1]), explosion.mLifeSpan - mLife, index);
		}
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
