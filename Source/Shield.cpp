#include "StdAfx.h"
#include "Shield.h"
#include "Damagable.h"
#include "Link.h"
#include "Drawlist.h"
#include "Resource.h"


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
, mCost(0.0f)
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
				child->QueryFloatAttribute("cost", &mCost);
			}
			break;
		}
	}

	return true;
}


Shield::Shield(void)
: mId(0)
, mAmmo(0)
{
}

Shield::Shield(const ShieldTemplate &aTemplate, unsigned int aId)
: mId(aId)
, mAmmo(0)
{
	// add a damage listener
	Database::Typed<Damagable::DamageListener> &listeners = Database::damagelistener.Open(mId);
	listeners.Put(0x337519b0 /* "shield" */, Damagable::DamageListener(this, &Shield::Damage));
	Database::damagelistener.Close(mId);

	// if the shield uses ammo...
	if (aTemplate.mCost)
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
			Database::Typed<Database::Typed<Resource::ChangeListener> > &changelisteners = Database::resourcechangelistener.Open(mAmmo);
			Database::Typed<Resource::ChangeListener> &changelistener = changelisteners.Open(aTemplate.mType);
			changelistener.Put(0x337519b0 /* "shield" */, Resource::ChangeListener(this, &Shield::Change));
			changelisteners.Close(aTemplate.mType);
			Database::resourcechangelistener.Close(mAmmo);
		}
	}
}

Shield::~Shield(void)
{
}


// resource change listener
void Shield::Change(unsigned int aId, unsigned int aSubId, unsigned int aSourceId, float aValue)
{
	// get the shield template
	const ShieldTemplate &shield = Database::shieldtemplate.Get(mId);

	// if using ammo...
	if (shield.mCost)
	{
		// get resource template
		const ResourceTemplate &resourcetemplate = Database::resourcetemplate.Get(mAmmo).Get(shield.mType);

		// save ratio into variables
		Database::Typed<float> &variables = Database::variable.Open(mId);
		variables.Put(0x337519b0 /* "shield" */, aValue / resourcetemplate.mMaximum);

		DebugPrint("shield=%s ammo=%08x value=%f\n", Database::name.Get(aId).c_str(), aSubId, aValue);
	}
}

// damage listener
void Shield::Damage(unsigned int aId, unsigned int aSourceId, float aDamage)
{
	if (aDamage <= 0)
		return;

	// get the shield template
	const ShieldTemplate &shield = Database::shieldtemplate.Get(mId);

	// if using ammo...
	if (shield.mCost)
	{
		// ammo resource (if any)
		Resource *resource = Database::resource.Get(mAmmo).Get(shield.mType);

		// damage absorb
		float absorb = aDamage;
		if (absorb * shield.mCost > resource->GetValue())
			absorb = resource->GetValue() / shield.mCost;
		DebugPrint("shield=%s damage=%f absorb=%f\n", Database::name.Get(aId).c_str(), aDamage, absorb);

		// use energy to offset damage (HACK)
		if (Damagable *damagable = Database::damagable.Get(mId))
		{
			damagable->Damage(aId, -absorb);
		}

		// expend ammo
		resource->Add(aSourceId, -absorb * shield.mCost);
	}
}
