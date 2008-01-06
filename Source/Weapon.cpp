#include "StdAfx.h"
#include "Weapon.h"
#include "Bullet.h"
#include "Entity.h"
#include "Controller.h"
#include "Link.h"

// gunner bullet physics
const float GUNNER_BULLET_SPEED = 800;


namespace Database
{
	Typed<WeaponTemplate> weapontemplate("weapontemplate");
	Typed<Weapon *> weapon("weapon");

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

bool WeaponTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x6f332041 /* "weapon" */)
		return false;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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
: Simulatable(0)
, mDelay(0.0f)
, mPhase(0)
{
}

Weapon::Weapon(const WeaponTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
, mDelay(0.0f)
, mPhase(aTemplate.mPhase)
{
}

Weapon::~Weapon(void)
{
}

// Weapon Configure
bool Weapon::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x6f332041 /* "weapon" */)
		return false;

	return true;
}

// Weapon Simulate
void Weapon::Simulate(float aStep)
{
	// get template data
	const WeaponTemplate &weapon = Database::weapontemplate.Get(id);

	// get controller
	unsigned int aOwnerId = Database::owner.Get(id);
	const Controller *controller = Database::controller.Get(aOwnerId);
	if (!controller)
		return;

	// advance fire timer
	mDelay -= aStep * weapon.mCycle;

	// if ready to fire...
	if (mDelay <= 0.0f)
	{
		// if triggered...
		if (controller->mFire)
		{
			// if firing on this phase...
			if (mPhase == 0)
			{
				// get the entity
				Entity *entity = Database::entity.Get(id);

				// instantiate a bullet
				Matrix2 transform(weapon.mOffset * entity->GetTransform());
				Database::Instantiate(weapon.mOrdnance,
					transform.Angle(), transform.p,
					transform.Rotate(weapon.mVelocity));

				// wrap around
				mPhase = weapon.mCycle - 1;
			}
			else
			{
				// advance phase
				--mPhase;
			}

			// update weapon delay
			mDelay += weapon.mDelay;
		}
		else
		{
			// clamp fire delay
			mDelay = 0.0f;
		}
	}
}
