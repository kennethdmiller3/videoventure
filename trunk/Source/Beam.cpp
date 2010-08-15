#include "StdAfx.h"
#include "Beam.h"
#include "Entity.h"
#include "Collidable.h"
#include "Damagable.h"
#include "Cancelable.h"
#include "Team.h"
#include "Renderable.h"
#include "Variable.h"
#include "Interpolator.h"


#ifdef USE_POOL_ALLOCATOR
// beam pool
static MemoryPool sPool(sizeof(Beam));
void *Beam::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Beam::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<BeamTemplate> beamtemplate(0x9a5ff3dc /* "beamtemplate" */);
	Typed<Beam *> beam(0xa75279fa /* "beam" */);
	Typed<Typed<std::vector<unsigned int> > > beamproperty(0xe6894b09 /* "beamproperty" */);

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
: mLifeSpan(0.0f)
, mDamage(0.0f)
, mRange(0.0f)
, mFilter(Collidable::GetDefaultFilter())
{
}

BeamTemplate::~BeamTemplate(void)
{
}


bool BeamTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	element->QueryFloatAttribute("life", &mLifeSpan);
	element->QueryFloatAttribute("damage", &mDamage);
	element->QueryFloatAttribute("range", &mRange);
	if (const char *spawn = element->Attribute("spawnonimpact"))
		mSpawnOnImpact = Hash(spawn);

	ConfigureFilterData(mFilter, element);

	if (element->FirstChildElement())
	{
		Database::Typed<std::vector<unsigned int> > &properties = Database::beamproperty.Open(id);
		for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
		{
			unsigned int propId = Hash(child->Value());
			std::vector<unsigned int> &buffer = properties.Open(propId);
			const char *names[1] = { "value" };
			const float data[1] = { 0.0f };
			ConfigureInterpolatorItem(child, buffer, 1, names, data);
			properties.Close(propId);
		}
		Database::beamproperty.Close(id);
	}

	return true;
}


Beam::Beam(void)
: Updatable(0)
, mLife(0)
{
	SetAction(Action(this, &Beam::Update));
}

Beam::Beam(const BeamTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mLife(aTemplate.mLifeSpan)
{
	SetAction(Action(this, &Beam::Update));
}

Beam::~Beam(void)
{
}

void Beam::Update(float aStep)
{
	// get beam template properties
	const BeamTemplate &beam = Database::beamtemplate.Get(mId);
	float curRange = beam.mRange;
	float curDamage = beam.mDamage;

	// get animated properties (if any)
	if (const Database::Typed<std::vector<unsigned int> > *properties = Database::beamproperty.Find(mId))
	{
		const std::vector<unsigned int> &rangebuffer = properties->Get(0xfadc0cd2 /* "range" */);
		if (!rangebuffer.empty())
		{
			int index = 0;
			ApplyInterpolator(&curRange, 1, rangebuffer[0], reinterpret_cast<const float * __restrict>(&rangebuffer[1]), beam.mLifeSpan - mLife, index);
		}
		const std::vector<unsigned int> &damagebuffer = properties->Get(0x59e94c40 /* "damage" */);
		if (!damagebuffer.empty())
		{
			int index = 0;
			ApplyInterpolator(&curDamage, 1, damagebuffer[0], reinterpret_cast<const float * __restrict>(&damagebuffer[1]), beam.mLifeSpan - mLife, index);
		}
	}

	// if the beam has range (it should!)
	if (curRange)
	{
		// get parent entity
		Entity *entity = Database::entity.Get(mId);

		// beam segment
		b2Vec2 start(entity->GetPosition());
		b2Vec2 end(entity->GetPosition() + Matrix2(entity->GetTransform()).y * curRange);

		// impact point
		float lambda = 1.0f;
		b2Vec2 normal(0.0f, 0.0f);
		b2Fixture *shape = NULL;

		// check for segment intersection
		unsigned int hitId = Collidable::TestSegment(start, end, beam.mFilter, mId, lambda, normal, shape);

		// save local endpoint for the renderer
		Database::Typed<float> &variables = Database::variable.Open(mId);
		variables.Put(0x9ea656c7 /* "beamend" */ + 0, 0.0f);
		variables.Put(0x9ea656c7 /* "beamend" */ + 1, lambda * curRange);
		variables.Put(0x9ea656c7 /* "beamend" */ + 2, 0.0f);
		variables.Put(0x9ea656c7 /* "beamend" */ + 3, 1.0f);
		Database::variable.Close(mId);

		// if the beam hit something...
		if (hitId != 0)
		{
				// if the recipient is damagable...
				// and not healing or the target is at max health...
			if (Damagable *damagable = Database::damagable.Get(hitId))
				{
					// get base damage
					float damage = curDamage;

					// if applying damage over time...
					if (beam.mLifeSpan > 0.0f)
					{
						// scale by time step
						damage *= aStep;
					}

					// limit healing
					if (damage < 0)
					{
						damage = std::max(damage, damagable->GetHealth() - Database::damagabletemplate.Get(hitId).mHealth);
					}

					// apply damage
					damagable->Damage(mId, damage);
			}

			if (curDamage >= 0)
			{
				// if the recipient is cancelable...
				if (Cancelable *cancelable = Database::cancelable.Get(hitId))
				{
					// apply cancel
					cancelable->Cancel(hitId, mId);
				}
			}
		}

		// if the beam stopped early...
		if (lambda < 1.0f)
		{
			// if spawning on impact...
			if (beam.mSpawnOnImpact)
			{
				// get the entity
				Entity *entity = Database::entity.Get(mId);
				if (entity)
				{
					// spawn template at the impact location
					unsigned int spawnId = Database::Instantiate(beam.mSpawnOnImpact, Database::owner.Get(mId), mId, 
						entity->GetAngle(), Vector2(start + lambda * (end - start)));

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
		// deactivate
		Deactivate();
	}
}
