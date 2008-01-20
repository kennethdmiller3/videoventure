#include "StdAfx.h"
#include "Weapon.h"
#include "Bullet.h"
#include "Entity.h"
#include "Controller.h"
#include "Link.h"
#include "Renderable.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// weapon pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Weapon));
void *Weapon::operator new(size_t aSize)
{
	return pool.malloc();
}
void Weapon::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<WeaponTemplate> weapontemplate(0xb1050fa7 /* "weapontemplate" */);
	Typed<Weapon *> weapon(0x6f332041 /* "weapon" */);

	namespace Loader
	{
		class WeaponLoader
		{
		public:
			WeaponLoader()
			{
				AddConfigure(0x6f332041 /* "weapon" */, Entry(this, &WeaponLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				WeaponTemplate &weapon = Database::weapontemplate.Open(aId);
				weapon.Configure(element);
				Database::weapontemplate.Close(aId);
			}
		}
		weaponloader;
	}

	namespace Initializer
	{
		class WeaponInitializer
		{
		public:
			WeaponInitializer()
			{
				AddActivate(0xb1050fa7 /* "weapontemplate" */, Entry(this, &WeaponInitializer::Activate));
				AddDeactivate(0xb1050fa7 /* "weapontemplate" */, Entry(this, &WeaponInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const WeaponTemplate &weapontemplate = Database::weapontemplate.Get(aId);
				Weapon *weapon = new Weapon(weapontemplate, aId);
				Database::weapon.Put(aId, weapon);
				weapon->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Weapon *weapon = Database::weapon.Get(aId))
				{
					delete weapon;
					Database::weapon.Delete(aId);
				}
			}
		}
		weaponinitializer;
	}
}


WeaponTemplate::WeaponTemplate(void)
: mOffset(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0))
, mInherit(1, 1)
, mVelocity(0, 0)
, mOrdnance(0)
, mDelay(1.0f)
, mPhase(0)
, mCycle(1)
{
}

WeaponTemplate::~WeaponTemplate(void)
{
}

bool WeaponTemplate::Configure(const TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x6f332041 /* "weapon" */)
		return false;

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x14c8d3ca /* "offset" */:
			{
				child->QueryFloatAttribute("x", &mOffset.p.x);
				child->QueryFloatAttribute("y", &mOffset.p.y);
				float angle = 0.0f;
				if (child->QueryFloatAttribute("angle", &angle) == TIXML_SUCCESS)
					mOffset = Matrix2(angle * float(M_PI) / 180.0f, mOffset.p);
			}
			break;

		case 0xca04efe0 /* "inherit" */:
			{
				child->QueryFloatAttribute("x", &mInherit.x);
				child->QueryFloatAttribute("y", &mInherit.y);
			}
			break;

		case 0x32741c32 /* "velocity" */:
			{
				child->QueryFloatAttribute("x", &mVelocity.x);
				child->QueryFloatAttribute("y", &mVelocity.y);
			}
			break;

		case 0x399bf05d /* "ordnance" */:
			{
				if (const char *ordnance = child->Attribute("name"))
					mOrdnance = Hash(ordnance);
			}
			break;

		case 0xac47e6f5 /* "shot" */:
			{
				child->QueryFloatAttribute("delay", &mDelay);
				child->QueryIntAttribute("phase", &mPhase);
				child->QueryIntAttribute("cycle", &mCycle);
			}
			break;
		}
	}

	return true;
}


Weapon::Weapon(void)
: Updatable(0)
, mTimer(0.0f)
, mPhase(0)
{
}

Weapon::Weapon(const WeaponTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mTimer(0.0f)
, mPhase(aTemplate.mPhase)
{
}

Weapon::~Weapon(void)
{
}

// Weapon Configure
bool Weapon::Configure(const TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x6f332041 /* "weapon" */)
		return false;

	return true;
}

// Weapon Update
void Weapon::Update(float aStep)
{
	// get controller
	unsigned int aOwnerId = Database::owner.Get(id);
	const Controller *controller = Database::controller.Get(aOwnerId);
	if (!controller)
	{
		Database::Delete(id);
		return;
	}

	// advance fire timer
	mTimer += aStep;

	// if triggered...
	if (controller->mFire)
	{
		// get template data
		const WeaponTemplate &weapon = Database::weapontemplate.Get(id);

		// if ready to fire...
		while (mTimer > 0.0f)
		{
			// if firing on this phase...
			if (mPhase == 0)
			{
				// get the entity
				Entity *entity = Database::entity.Get(id);

				// instantiate a bullet
				Matrix2 transform(weapon.mOffset * entity->GetInterpolatedTransform(mTimer / aStep));
				Vector2 velocity(transform.Rotate(weapon.mInherit * transform.Unrotate(entity->GetVelocity()) + weapon.mVelocity));
				unsigned int ordId = Database::Instantiate(weapon.mOrdnance,
					transform.Angle(), transform.p, velocity, 0);

				// set fractional turn
				if (Renderable *renderable = Database::renderable.Get(ordId))
					renderable->SetFraction(mTimer / aStep);

				// wrap around
				mPhase = weapon.mCycle - 1;
			}
			else
			{
				// advance phase
				--mPhase;
			}

			// update weapon delay
			mTimer -= weapon.mDelay / weapon.mCycle;
		}
	}
	else
	{
		if (mTimer > 0.0f)
		{
			// clamp fire delay
			mTimer = 0.0f;
		}
	}
}
