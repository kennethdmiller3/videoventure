#include "StdAfx.h"
#include "Shield.h"
#include "Damagable.h"
#include "Link.h"
#include "Drawlist.h"
#include "Resource.h"
#include "Entity.h"


#ifdef USE_POOL_ALLOCATOR
// shield pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Shield));
void *Shield::operator new(size_t aSize)
{
	return pool.malloc();
}
void Shield::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<ShieldTemplate> shieldtemplate(0xf7eb1c5a /* "shieldtemplate" */);
	Typed<Shield *> shield(0x337519b0 /* "shield" */);

	namespace Loader
	{
		class ShieldLoader
		{
		public:
			ShieldLoader()
			{
				AddConfigure(0x337519b0 /* "shield" */, Entry(this, &ShieldLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				ShieldTemplate &shield = Database::shieldtemplate.Open(aId);
				shield.Configure(element);
				Database::shieldtemplate.Close(aId);
			}
		}
		shieldloader;
	}

	namespace Initializer
	{
		class ShieldInitializer
		{
		public:
			ShieldInitializer()
			{
				AddActivate(0xf7eb1c5a /* "shieldtemplate" */, Entry(this, &ShieldInitializer::Activate));
				AddDeactivate(0xf7eb1c5a /* "shieldtemplate" */, Entry(this, &ShieldInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const ShieldTemplate &shieldtemplate = Database::shieldtemplate.Get(aId);
				Shield *shield = new Shield(shieldtemplate, aId);
				Database::shield.Put(aId, shield);
			}

			void Deactivate(unsigned int aId)
			{
				if (Shield *shield = Database::shield.Get(aId))
				{
					delete shield;
					Database::shield.Delete(aId);
				}
			}
		}
		shieldinitializer;
	}
}


ShieldTemplate::ShieldTemplate(void)
: mType(0U)
, mBase(0.0f)
, mScale(1.0f)
, mLimit(FLT_MAX)
, mCost(0.0f)
, mSpawn(0U)
, mInvulnerable(0.0f)
{
}

ShieldTemplate::~ShieldTemplate(void)
{
}

bool ShieldTemplate::Configure(const TiXmlElement *element)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x5b9b0daf /* "ammo" */:
			{
				if (const char *type = child->Attribute("type"))
					mType = Hash(type);
				child->QueryFloatAttribute("cost", &mBase);
			}
			break;

		case 0xb65dd078 /* "absorb" */:
			{
				child->QueryFloatAttribute("scale", &mScale);
				child->QueryFloatAttribute("limit", &mLimit);
				child->QueryFloatAttribute("cost", &mCost);
				if (const char *spawn = child->Attribute("spawn"))
					mSpawn = Hash(spawn);
			}
			break;

		case 0x72e2ca22 /* "invulnerable" */:
			{
				child->QueryFloatAttribute("time", &mInvulnerable);
			}
			break;
		}
	}

	return true;
}


Shield::Shield(void)
: mId(0)
, mAmmo(0)
, mTurn(~0U)
{
}

Shield::Shield(const ShieldTemplate &aTemplate, unsigned int aId)
: mId(aId)
, mAmmo(0)
, mTurn(~0U)
{
	// add a damage listener
	Damagable::DamageSignal &signal = Database::damagesignal.Open(mId);
	signal.Connect(this, &Shield::Damage);
	Database::damagesignal.Close(mId);

	// if the shield uses ammo...
	if (aTemplate.mType)
	{
		// find the specified resource
		mAmmo = FindResource(aId, aTemplate.mType);

		// if found...
		if (mAmmo)
		{
			// get resource template
			const ResourceTemplate &resourcetemplate = Database::resourcetemplate.Get(mAmmo).Get(aTemplate.mType);

			// save ratio into variables
			Database::Typed<float> &variables = Database::variable.Open(mId);
			variables.Put(0x337519b0 /* "shield" */, resourcetemplate.mInitial / resourcetemplate.mMaximum);

			// add a resource listener
			Database::Typed<Resource::ChangeSignal> &changesignals = Database::resourcechange.Open(mAmmo);
			Resource::ChangeSignal &changesignal = changesignals.Open(aTemplate.mType);
			changesignal.Connect(this, &Shield::Change);
			changesignals.Close(aTemplate.mType);
			Database::resourcechange.Close(mAmmo);
		}
	}
}

Shield::~Shield(void)
{
	// remove the damage listener
	Damagable::DamageSignal &signal = Database::damagesignal.Open(mId);
	signal.Disconnect(this, &Shield::Damage);
	Database::damagesignal.Close(mId);
}


// resource change listener
void Shield::Change(unsigned int aId, unsigned int aSubId, unsigned int aSourceId, float aValue)
{
	// get the shield template
	const ShieldTemplate &shield = Database::shieldtemplate.Get(mId);

	// if using ammo...
	if (shield.mType)
	{
		// get resource template
		const ResourceTemplate &resourcetemplate = Database::resourcetemplate.Get(mAmmo).Get(shield.mType);

		// save ratio into variables
		Database::Typed<float> &variables = Database::variable.Open(mId);
		variables.Put(0x337519b0 /* "shield" */, aValue / resourcetemplate.mMaximum);

#ifdef DEBUG_SHIELD
		DebugPrint("shield=%s ammo=%08x value=%f\n", Database::name.Get(aId).c_str(), aSubId, aValue);
#endif
	}
}

// damage listener
void Shield::Damage(unsigned int aId, unsigned int aSourceId, float aDamage)
{
	if (aDamage <= 0)
		return;

	// get the shield template
	const ShieldTemplate &shield = Database::shieldtemplate.Get(mId);

	// if within invulnerability window...
	if ((sim_turn - mTurn) * sim_step < shield.mInvulnerable)
	{
		// absorb 100%
		if (Damagable *damagable = Database::damagable.Get(mId))
		{
			damagable->Damage(aId, -aDamage);
		}
		return;
	}

	// if using ammo...
	if (shield.mType)
	{
		// ammo resource (if any)
		Resource *resource = Database::resource.Get(mAmmo).Get(shield.mType);

		// only pay base cost once per turn
		float base = (sim_turn == mTurn) ? 0.0f : shield.mBase;

		// do nothing if not enough ammo to meet base
		if (base > resource->GetValue())
			return;

		// damage absorb
		float absorb = std::min(aDamage * shield.mScale, shield.mLimit);
		if (shield.mCost != 0.0f && base + absorb * shield.mCost > resource->GetValue())
			absorb = (resource->GetValue() - base) / shield.mCost;
#ifdef DEBUG_SHIELD
		DebugPrint("shield=%s damage=%f absorb=%f\n", Database::name.Get(aId).c_str(), aDamage, absorb);
#endif

		// use energy to offset damage (HACK)
		if (Damagable *damagable = Database::damagable.Get(mId))
		{
			damagable->Damage(aId, -absorb);
		}

		// expend ammo
		resource->Add(aSourceId, -(base + absorb * shield.mCost));

		// if spawning
		if ((shield.mSpawn) && (sim_turn != mTurn))
		{
			Entity *entity = Database::entity.Get(mId);
			Database::Instantiate(shield.mSpawn, Database::owner.Get(mId), mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega(), true);
		}

		// mark trigger turn
		mTurn = sim_turn;
	}
}
