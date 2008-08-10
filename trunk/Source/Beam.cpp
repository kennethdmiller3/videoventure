#include "StdAfx.h"
#include "Beam.h"
#include "Collidable.h"
#include "Damagable.h"
#include "Team.h"
#include "Drawlist.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// beam pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Beam));
void *Beam::operator new(size_t aSize)
{
	return pool.malloc();
}
void Beam::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<BeamTemplate> beamtemplate(0x9a5ff3dc /* "beamtemplate" */);
	Typed<Beam *> beam(0xa75279fa /* "beam" */);

	namespace Loader
	{
		class BeamLoader
		{
		public:
			BeamLoader()
			{
				AddConfigure(0xa75279fa /* "beam" */, Entry(this, &BeamLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				BeamTemplate &beam = Database::beamtemplate.Open(aId);
				beam.Configure(element, aId);
				Database::beamtemplate.Close(aId);
			}
		}
		beamloader;
	}

	namespace Initializer
	{
		class BeamInitializer
		{
		public:
			BeamInitializer()
			{
				AddActivate(0x9a5ff3dc /* "beamtemplate" */, Entry(this, &BeamInitializer::Activate));
				AddDeactivate(0x9a5ff3dc /* "beamtemplate" */, Entry(this, &BeamInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const BeamTemplate &beamtemplate = Database::beamtemplate.Get(aId);
				Beam *beam = new Beam(beamtemplate, aId);
				Database::beam.Put(aId, beam);
				beam->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Beam *beam = Database::beam.Get(aId))
				{
					delete beam;
					Database::beam.Delete(aId);
				}
			}
		}
		beaminitializer;
	}
};


BeamTemplate::BeamTemplate(void)
: mLifeSpan(0.25f)
, mCategoryBits(0xFFFF)
, mMaskBits(0xFFFF)
, mDamage(0.0f)
, mDamageRate(0.0f)
, mRange(0.0f)
, mSpawnOnExpire(0)
{
}

BeamTemplate::~BeamTemplate(void)
{
}


bool BeamTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	element->QueryFloatAttribute("life", &mLifeSpan);
	element->QueryFloatAttribute("damage", &mDamage);
	element->QueryFloatAttribute("damagerate", &mDamageRate);
	element->QueryFloatAttribute("range", &mRange);
	if (const char *spawn = element->Attribute("spawnonexpire"))
		mSpawnOnExpire = Hash(spawn);
	if (const char *spawn = element->Attribute("spawnonimpact"))
		mSpawnOnImpact = Hash(spawn);

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


Beam::Beam(void)
: Updatable(0)
, mLife(0)
{
}

Beam::Beam(const BeamTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mLife(aTemplate.mLifeSpan)
{
}

Beam::~Beam(void)
{
}

void Beam::Update(float aStep)
{
	// if the beam has range (it should!)
	// the beam does damage and this is the first step, or
	// the beam has a damage rate
	const BeamTemplate &beam = Database::beamtemplate.Get(mId);
	if (beam.mRange > 0.0f)
	{
		// get parent entity
		Entity *entity = Database::entity.Get(mId);

		// beam segment
		b2Segment segment;
		segment.p1 = entity->GetPosition();
		segment.p2 = entity->GetPosition() + entity->GetTransform().y * beam.mRange;

		// impact point
		float lambda = 1.0f;
		b2Vec2 normal(0.0f, 0.0f);
		b2Shape *shape = NULL;

		// check for segment intersection
		unsigned int hitId = Collidable::TestSegment(segment, 0.0f, beam.mCategoryBits, beam.mMaskBits, lambda, normal, shape);

		// save local endpoint for the renderer
		Database::Typed<float> &variables = Database::variable.Open(mId);
		variables.Put(0x9ea656c7 /* "beamend" */ + 0, 0.0f);
		variables.Put(0x9ea656c7 /* "beamend" */ + 1, lambda * beam.mRange);
		variables.Put(0x9ea656c7 /* "beamend" */ + 2, 0.0f);
		variables.Put(0x9ea656c7 /* "beamend" */ + 3, 1.0f);
		Database::variable.Close(mId);

		// if applying damage...
		if (hitId != 0 && ((beam.mDamage != 0) && (mLife == beam.mLifeSpan)) || (beam.mDamageRate != 0))
		{
			// if the recipient is damagable...
			// and not healing or the target is at max health...
			Damagable *damagable = Database::damagable.Get(hitId);
			if (damagable)
			{
				// get base damage
				float damage = beam.mDamage * (mLife == beam.mLifeSpan) + beam.mDamageRate * aStep;

				// limit healing
				if (damage < 0)
				{
					damage = std::max(damage, damagable->GetHealth() - Database::damagabletemplate.Get(hitId).mHealth);
				}

				// apply damage
				damagable->Damage(mId, damage);
			}

			// if spawning on impact...
			if (beam.mSpawnOnImpact)
			{
				// get the entity
				Entity *entity = Database::entity.Get(mId);
				if (entity)
				{
					// spawn template at the impact location
					unsigned int spawnId = Database::Instantiate(beam.mSpawnOnImpact, Database::owner.Get(mId),
						entity->GetAngle(), Vector2(segment.p1 + lambda * (segment.p2 - segment.p1)));

					// copy renderable fractional turn
					if (Renderable *renderable = Database::renderable.Get(spawnId))
						if (Renderable *myrenderable = Database::renderable.Get(mId))
							renderable->SetFraction(myrenderable->GetFraction());
				}
			}
		}
	}

	// advance life timer
	mLife -= aStep;

	// if expired...
	if (mLife <= 0)
	{
		// if spawning on expire...
		const BeamTemplate &beam = Database::beamtemplate.Get(mId);
		if (beam.mSpawnOnExpire)
		{
#ifdef USE_CHANGE_DYNAMIC_TYPE
			// change dynamic type
			Database::Deactivate(mId);
			Database::parent.Put(mId, beam.mSpawnOnExpire);
			Database::Activate(mId);
#else
			// get the entity
			Entity *entity = Database::entity.Get(mId);
			if (entity)
			{
				// spawn template at the entity location
				Database::Instantiate(beam.mSpawnOnExpire, Database::owner.Get(mId), entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
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
