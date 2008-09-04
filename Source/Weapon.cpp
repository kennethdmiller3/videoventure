#include "StdAfx.h"
#include "Weapon.h"
#include "Bullet.h"
#include "Entity.h"
#include "Controller.h"
#include "Link.h"
#include "Renderable.h"
#include "Sound.h"
#include "Resource.h"


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


class WeaponTracker
{
public:
	unsigned int mId;

	WeaponTracker(unsigned int aId = 0)
		: mId(aId)
	{
		if (Weapon *weapon = Database::weapon.Get(mId))
			weapon->Track(1);
	}

	WeaponTracker(const WeaponTracker &aSource)
		: mId(aSource.mId)
	{
		if (Weapon *weapon = Database::weapon.Get(mId))
			weapon->Track(1);
	}

	~WeaponTracker()
	{
		if (Weapon *weapon = Database::weapon.Get(mId))
			weapon->Track(-1);
	}

	const WeaponTracker &operator=(const WeaponTracker &aSource)
	{
		if (Weapon *weapon = Database::weapon.Get(mId))
			weapon->Track(-1);
		if (Weapon *weapon = Database::weapon.Get(mId))
			weapon->Track(1);
		return *this;
	}
};

namespace Database
{
	Typed<WeaponTemplate> weapontemplate(0xb1050fa7 /* "weapontemplate" */);
	Typed<Weapon *> weapon(0x6f332041 /* "weapon" */);
	Typed<WeaponTracker> weapontracker(0x49c0728f /* "weapontracker" */);

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

				// TO DO: check to make sure this does not have an order dependency
				weapon->SetControl(aId);
				for (unsigned int aControlId = aId; aControlId != 0; aControlId = Database::backlink.Get(aControlId))
				{
					if (Database::controller.Find(aControlId))
					{
						weapon->SetControl(aControlId);
						break;
					}
				}
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
, mScatter(0, 0)
, mSpread(0)
, mOrdnance(0)
, mFlash(0)
, mChannel(0)
, mDelay(1.0f)
, mPhase(0)
, mCycle(1)
, mTrack(0)
, mType(0U)
, mCost(0.0f)
{
}

WeaponTemplate::~WeaponTemplate(void)
{
}

bool WeaponTemplate::Configure(const TiXmlElement *element)
{
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

		case 0xcab7a341 /* "scatter" */:
			{
				child->QueryFloatAttribute("x", &mScatter.x);
				child->QueryFloatAttribute("y", &mScatter.y);
				if (child->QueryFloatAttribute("angle", &mSpread) == TIXML_SUCCESS)
					mSpread *= float(M_PI) / 180.0f;
			}
			break;

		case 0x399bf05d /* "ordnance" */:
			{
				if (const char *ordnance = child->Attribute("name"))
					mOrdnance = Hash(ordnance);
			}
			break;

		case 0xaf85ad29 /* "flash" */:
			{
				if (const char *flash = child->Attribute("name"))
					mFlash = Hash(flash);
			}
			break;

		case 0x75413203 /* "trigger" */:
			{
				if (child->QueryIntAttribute("channel", &mChannel) == TIXML_SUCCESS)
					--mChannel;
				// TO DO: support single/automatic/charge
			}
			break;

		case 0xac47e6f5 /* "shot" */:
			{
				child->QueryFloatAttribute("delay", &mDelay);
				child->QueryIntAttribute("phase", &mPhase);
				child->QueryIntAttribute("cycle", &mCycle);
				child->QueryIntAttribute("track", &mTrack);
			}
			break;

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


Weapon::Weapon(void)
: Updatable(0)
, mControlId(0)
, mChannel(0)
, mTrack(0)
, mTimer(0.0f)
, mPhase(0)
{
	SetAction(Action(this, &Weapon::Update));
}

Weapon::Weapon(const WeaponTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mControlId(0)
, mChannel(aTemplate.mChannel)
, mTrack(0)
, mTimer(0.0f)
, mPhase(aTemplate.mPhase)
, mAmmo(0)
{
	SetAction(Action(this, &Weapon::Update));

	// if the weapon uses ammo...
	if (aTemplate.mCost)
	{
		// check the weapon and backlinks
		for (unsigned int aId = mId; aId; aId = Database::backlink.Get(aId))
		{
			// if the entity has a matching resource...
			if (Database::resource.Get(aId).Get(aTemplate.mType))
			{
				// use that
				mAmmo = aId;
			}
		}

		// if no ammo found
		if (!mAmmo)
		{
			// get the owner (player)
			unsigned int owner = Database::owner.Get(mId);

			// if the owner has a matching resource...
			if (Database::resource.Get(owner).Get(aTemplate.mType))
			{
				// use that
				mAmmo = owner;
			}
		}
	}
}

Weapon::~Weapon(void)
{
}

// Weapon Configure
bool Weapon::Configure(const TiXmlElement *element)
{
	return true;
}

// Weapon Update
void Weapon::Update(float aStep)
{
	// get controller
	const Controller *controller = Database::controller.Get(mControlId);
	if (!controller)
		return;

	// advance fire timer
	mTimer += aStep;

	// if triggered...
	if (controller->mFire[mChannel])
	{
		// get template data
		const WeaponTemplate &weapon = Database::weapontemplate.Get(mId);

		// if ready to fire...
		while (mTimer > 0.0f && (!weapon.mTrack || mTrack < weapon.mTrack))
		{
			Resource *resource = NULL;

			// if the weapon uses ammo...
			if (weapon.mCost)
			{
				// find ammo resource
				for (unsigned int aId = mId; aId; aId = Database::backlink.Get(aId))
				{
					resource = Database::resource.Get(aId).Get(weapon.mType);
					if (resource)
						break;
				}

				// check owner if not found
				if (!resource)
					resource = Database::resource.Get(Database::owner.Get(mId)).Get(weapon.mType);

				// don't fire if out of ammo
				if (resource && weapon.mCost > resource->GetValue())
					break;
			}

			// if firing on this phase...
			if (mPhase == 0)
			{
				// deduct ammo
				if (resource)
					resource->Add(mId, -weapon.mCost);

				// get the entity
				Entity *entity = Database::entity.Get(mId);

				// start the sound cue
				PlaySound(mId, 0x8eab16d9 /* "fire" */);

				// interpolated offset
				Matrix2 transform(weapon.mOffset * entity->GetInterpolatedTransform(mTimer / aStep));

				if (weapon.mFlash)
				{
					// instantiate a flash
					unsigned int flashId = Database::Instantiate(weapon.mFlash, Database::owner.Get(mId),
						transform.Angle(), transform.p, entity->GetVelocity(), entity->GetOmega());

					// set fractional turn
					if (Renderable *renderable = Database::renderable.Get(flashId))
						renderable->SetFraction(mTimer / aStep);

					// link it (HACK)
					LinkTemplate linktemplate;
					linktemplate.mOffset = weapon.mOffset;
					linktemplate.mSub = flashId;
					linktemplate.mSecondary = flashId;
					Link *link = new Link(linktemplate, mId);
					Database::Typed<Link *> &links = Database::link.Open(mId);
					links.Put(flashId, link);
					Database::link.Close(mId);
					link->Activate();
				}

				if (weapon.mOrdnance)
				{
					// instantiate a bullet
					if (weapon.mSpread)
						transform = Matrix2(transform.Angle() + weapon.mSpread * (RandFloat() - RandFloat()), transform.p);
					const Vector2 inheritvelocity(weapon.mInherit * transform.Unrotate(entity->GetVelocity()));
					const Vector2 scattervelocity(weapon.mScatter.x ? weapon.mScatter.x * (RandFloat() - RandFloat()) : 0, weapon.mScatter.y ? weapon.mScatter.y * (RandFloat() - RandFloat()) : 0);
					const Vector2 velocity(transform.Rotate(weapon.mVelocity + inheritvelocity + scattervelocity));
					unsigned int ordId = Database::Instantiate(weapon.mOrdnance, Database::owner.Get(mId), transform.Angle(), transform.p, velocity, 0);
#ifdef DEBUG_WEAPON_CREATE_ORDNANCE
					DebugPrint("ordnance=\"%s\" owner=\"%s\"\n",
						Database::name.Get(ordId).c_str(),
						Database::name.Get(Database::owner.Get(ordId)).c_str());
#endif

					// set fractional turn
					if (Renderable *renderable = Database::renderable.Get(ordId))
						renderable->SetFraction(mTimer / aStep);

					// if tracking....
					if (weapon.mTrack)
					{
						// add a tracker
						Database::weapontracker.Put(ordId, WeaponTracker(mId));
					}
				}

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
