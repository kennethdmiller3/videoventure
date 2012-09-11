#include "StdAfx.h"
#include "Weapon.h"
#include "Bullet.h"
#include "Entity.h"
#include "Controller.h"
#include "Link.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Sound.h"
#include "Resource.h"
#include "Interpolator.h"


#ifdef USE_POOL_ALLOCATOR
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
				weapon.Configure(element, aId);
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
				AddPostActivate(0xb1050fa7 /* "weapontemplate" */, Entry(this, &WeaponInitializer::PostActivate));
				AddDeactivate(0xb1050fa7 /* "weapontemplate" */, Entry(this, &WeaponInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const WeaponTemplate &weapontemplate = Database::weapontemplate.Get(aId);
				Weapon *weapon = new Weapon(weapontemplate, aId);
				Database::weapon.Put(aId, weapon);
				weapon->SetControl(aId);
			}

			void PostActivate(unsigned int aId)
			{
				Weapon *weapon = Database::weapon.Get(aId);
				for (unsigned int aControlId = aId; aControlId != 0; aControlId = Database::backlink.Get(aControlId))
				{
					if (const Controller *controller = Database::controller.Get(aControlId))
					{
						weapon->SetControl(aControlId);
						weapon->SetPrevFire(controller->mFire[Database::weapontemplate.Get(aId).mChannel]);
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
: mOffset(0, Vector2(0, 0))
, mScatter(0, Vector2(0, 0))
, mInherit(0, Vector2(1, 1))
, mVelocity(0, Vector2(0, 0))
, mVariance(0, Vector2(0, 0))
, mAim(0, 0)
, mRecoil(0)
, mOrdnance(0)
, mFlash(0)
, mChannel(0)
, mDelay(1.0f)
, mPhase(0)
, mCycle(1)
, mTrack(0)
, mBurstStart(0.0f)
, mBurstLength(1)
, mBurstDelay(0.0f)
, mSalvoShots(1)
, mType(0U)
, mCost(0.0f)
{
}

WeaponTemplate::~WeaponTemplate(void)
{
}

bool WeaponTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x14c8d3ca /* "offset" */:
			{
				if (child->QueryFloatAttribute("angle", &mOffset.a) == TIXML_SUCCESS)
					mOffset.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mOffset.p.x);
				child->QueryFloatAttribute("y", &mOffset.p.y);
			}
			break;

		case 0xcab7a341 /* "scatter" */:
			{
				if (child->QueryFloatAttribute("angle", &mScatter.a) == TIXML_SUCCESS)
					mScatter.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mScatter.p.x);
				child->QueryFloatAttribute("y", &mScatter.p.y);
			}
			break;

		case 0xca04efe0 /* "inherit" */:
			{
				if (child->QueryFloatAttribute("angle", &mInherit.a) == TIXML_SUCCESS)
					mInherit.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mInherit.p.x);
				child->QueryFloatAttribute("y", &mInherit.p.y);
			}
			break;

		case 0x32741c32 /* "velocity" */:
			{
				if (child->QueryFloatAttribute("angle", &mVelocity.a) == TIXML_SUCCESS)
					mVelocity.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVelocity.p.x);
				child->QueryFloatAttribute("y", &mVelocity.p.y);

				// if the property has keyframes...
				// (TO DO: handle this in a smarter way)
				if (child->FirstChildElement())
				{
					// process the interpolator item
					Database::Typed<std::vector<unsigned int> > &properties = Database::weaponproperty.Open(aId);
					std::vector<unsigned int> &buffer = properties.Open(aPropId);
					const char *names[] = { "angle", "x", "y" };
					ConfigureInterpolatorItem(child, buffer, sizeof(mVelocity)/sizeof(float), names, (float *)(&mVelocity));
					properties.Close(aPropId);
					Database::weaponproperty.Close(aId);
				}
			}
			break;

		case 0x383251f6 /* "aim" */:
			{
				element->QueryFloatAttribute("x", &mAim.x);
				element->QueryFloatAttribute("y", &mAim.y);
			}
			break;

		case 0x0dd0b0be /* "variance" */:
			{
				if (child->QueryFloatAttribute("angle", &mVariance.a) == TIXML_SUCCESS)
					mVariance.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVariance.p.x);
				child->QueryFloatAttribute("y", &mVariance.p.y);
			}
			break;

		case 0x63734e77 /* "recoil" */:
			{
				child->QueryFloatAttribute("value", &mRecoil);
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
				// TO DO: support single/automatic/charge
				switch (Hash(child->Attribute("type")))
				{
				case 0xadc649b8 /* "hold" */:		mTrigger = TRIGGER_HOLD; break;
				case 0x01fcf9b4 /* "press" */:		mTrigger = TRIGGER_PRESS; break;
				case 0x1036ae7e /* "release" */:	mTrigger = TRIGGER_RELEASE; break;
				};
				if (child->QueryIntAttribute("channel", &mChannel) == TIXML_SUCCESS)
					--mChannel;
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

		case 0xfd3600a1 /* "burst" */:
			{
				child->QueryFloatAttribute("start", &mBurstStart);
				child->QueryIntAttribute("length", &mBurstLength);
				child->QueryFloatAttribute("delay", &mBurstDelay);
			}
			break;

		case 0x8ac0eddc /* "salvo" */:
			{
				child->QueryIntAttribute("shots", &mSalvoShots);
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
, mBurst(0)
, mTimer(0.0f)
, mPhase(0)
, mAmmo(0)
{
	SetAction(Action(this, &Weapon::Update));
}

Weapon::Weapon(const WeaponTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mControlId(0)
, mChannel(aTemplate.mChannel)
, mPrevFire(0.0f)
, mTrack(0)
, mBurst(0)
, mTimer(0.0f)
, mPhase(aTemplate.mPhase)
, mAmmo(0)
{
	SetAction(Action(this, &Weapon::Update));

	// if the weapon uses ammo...
	if (aTemplate.mCost)
	{
		// find the specified resource
		mAmmo = FindResource(aId, aTemplate.mType);
	}
}

Weapon::~Weapon(void)
{
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

	// get template data
	const WeaponTemplate &weapon = Database::weapontemplate.Get(mId);

	// get trigger value
	float fire = controller->mFire[mChannel];
	bool trigger;
	switch (weapon.mTrigger)
	{
	default:
	case WeaponTemplate::TRIGGER_HOLD: trigger = fire != 0; break;
	case WeaponTemplate::TRIGGER_PRESS: trigger = fire != 0 && mPrevFire == 0; break;
	case WeaponTemplate::TRIGGER_RELEASE: trigger = fire == 0 && mPrevFire != 0; break;
	}
	mPrevFire = fire;

	// if triggered...
	if (trigger)
	{
		// if not busy
		if (mBurst <= 0 && mTimer > -0.001f && (!weapon.mTrack || mTrack < weapon.mTrack))
		{
			// if firing on this phase...
			if (mPhase == 0)
			{
				Resource *resource = NULL;

				// if using ammo
				if (weapon.mCost)
				{
					// ammo resource (if any)
					resource = Database::resource.Get(mAmmo).Get(weapon.mType);
				}

				// if enough ammo...
				if (!resource || weapon.mCost <= resource->GetValue())
				{
					// deduct ammo
					if (resource)
						resource->Add(mId, -weapon.mCost);

					// start a new burst
					mTimer -= weapon.mBurstStart;
					mBurst = weapon.mBurstLength;
				}
				else
				{
					// start "empty" sound cue
					PlaySoundCue(mId, 0x18a7beee /* "empty" */);
				}

				// wrap around
				mPhase = weapon.mCycle - 1;
			}
			else
			{
				// advance phase
				--mPhase;

				// wait for next phase
				mTimer -= weapon.mDelay / weapon.mCycle;
			}
		}
	}

	// if ready to fire...
	while (mBurst > 0 && mTimer > -0.001f && (!weapon.mTrack || mTrack < weapon.mTrack))
	{
		// deduct a burst
		--mBurst;

		// get the entity
		Entity *entity = Database::entity.Get(mId);

		// start the "fire" sound cue
		PlaySoundCue(mId, 0x8eab16d9 /* "fire" */);

		// interpolated transform
		Transform2 basetransform(entity->GetInterpolatedTransform(mTimer / aStep));

		for (int salvo = 0; salvo < weapon.mSalvoShots; ++salvo)
		{
			// get local position
			Transform2 position(weapon.mOffset);

			// apply transform offset
			Transform2 transform(position * basetransform);

			if (weapon.mRecoil)
			{
				// apply recoil force
				for (unsigned int id = mId; id != 0; id = Database::backlink.Get(id))
				{
					if (Collidable *collidable = Database::collidable.Get(id))
					{
						collidable->GetBody()->ApplyImpulse(transform.Rotate(Vector2(0, -weapon.mRecoil)), transform.p);
						break;
					}
				}
			}

			if (weapon.mFlash)
			{
				// instantiate a flash
				unsigned int flashId = Database::Instantiate(weapon.mFlash, Database::owner.Get(mId), mId,
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
				// TO DO: consolidate this with similar spawn patterns (Graze, Spawner)

				// apply position scatter
				position.a += Random::Value(0.0f, weapon.mScatter.a);
				position.p.x += Random::Value(0.0f, weapon.mScatter.p.x);
				position.p.y += Random::Value(0.0f, weapon.mScatter.p.y);

				// get world position
				position *= basetransform;

				// get local velocity
				Transform2 velocity(entity->GetOmega(), position.Unrotate(entity->GetVelocity()));

				// apply velocity inherit
				velocity.a *= weapon.mInherit.a;
				velocity.p.x *= weapon.mInherit.p.x;
				velocity.p.y *= weapon.mInherit.p.y;

				// apply velocity add
				velocity.a += weapon.mVelocity.a;
				velocity.p.x += weapon.mVelocity.p.x;
				velocity.p.y += weapon.mVelocity.p.y;

				// apply velocity variance
				velocity.a += Random::Value(0.0f, weapon.mVariance.a);
				velocity.p.x += Random::Value(0.0f, weapon.mVariance.p.x);
				velocity.p.y += Random::Value(0.0f, weapon.mVariance.p.y);

				// apply velocity aim
				velocity.p.x += controller->mAim.x * weapon.mAim.x;
				velocity.p.y += controller->mAim.y * weapon.mAim.y;

				// get world velocity
				velocity.p = position.Rotate(velocity.p);

				// instantiate a bullet
				unsigned int ordId = Database::Instantiate(weapon.mOrdnance, Database::owner.Get(mId), mId, position.a, position.p, velocity.p, velocity.a);
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
		}

		// update weapon delay
		if (mBurst > 0)
			mTimer -= weapon.mBurstDelay;
		else
			mTimer -= (weapon.mDelay - weapon.mBurstStart - weapon.mBurstDelay * (weapon.mBurstLength - 1)) / weapon.mCycle;
	}

	if (mTimer > 0.0f)
	{
		// clamp fire delay
		mTimer = 0.0f;
	}
}
