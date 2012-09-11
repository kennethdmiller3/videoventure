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

#include "ExpressionConfigure.h"
#include "Drawlist.h" // for variable

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
: mAim(0, 0)
, mRecoil(0)
, mChannel(0)
, mDelay(0.0f)
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

// actions

static const char * const sTransformNames[] = { "x", "y", "angle", ""};
static const float sTransformDefault[] = { 0.0f, 0.0f, 0.0f, 0.0f };

template<> inline Transform2 Cast<Transform2, __m128>(__m128 i)
{
	return Transform2(reinterpret_cast<const float * __restrict>(&i)[2],
		Vector2(reinterpret_cast<const float * __restrict>(&i)[0], reinterpret_cast<const float * __restrict>(&i)[1]));
}

static void WeaponWait(EntityContext &aContext)
{
	float delay = Expression::Evaluate<float>(aContext);
	aContext.mParam -= delay;
}

static void WeaponRecoil(EntityContext &aContext)
{
	float recoil(Expression::Evaluate<float>(aContext));
	if (recoil)
	{
		// get the entity
		Entity *entity = Database::entity.Get(aContext.mId);

		// interpolated transform
		Transform2 transform(entity->GetInterpolatedTransform(aContext.mParam / sim_step));

		// apply recoil force
		for (unsigned int id = aContext.mId; id != 0; id = Database::backlink.Get(id))
		{
			if (Collidable *collidable = Database::collidable.Get(id))
			{
				collidable->GetBody()->ApplyImpulse(transform.Rotate(Vector2(0, -recoil)), transform.p);
				break;
			}
		}
	}
}

static void WeaponFlash(EntityContext &aContext)
{
	unsigned int flash(Expression::Read<unsigned int>(aContext));
	Transform2 position(Cast<Transform2, __m128>(Expression::Evaluate<__m128>(aContext)));
	position.a *= float(M_PI) / 180.0f;

	// get the entity
	Entity *entity = Database::entity.Get(aContext.mId);

	// interpolated transform
	Transform2 basetransform(entity->GetInterpolatedTransform(aContext.mParam / sim_step));

	// get world position
	Transform2 transform(position * basetransform);

	// instantiate a flash
	if (unsigned int flashId = Database::Instantiate(flash, Database::owner.Get(aContext.mId), aContext.mId,
		transform.a, transform.p, entity->GetVelocity(), entity->GetOmega()))
	{
		// set fractional turn
		if (Renderable *renderable = Database::renderable.Get(flashId))
			renderable->SetFraction(aContext.mParam / sim_step);

		// link it (HACK)
		LinkTemplate linktemplate;
		linktemplate.mOffset = position;
		linktemplate.mSub = flashId;
		linktemplate.mSecondary = flashId;
		Link *link = new Link(linktemplate, aContext.mId);
		Database::Typed<Link *> &links = Database::link.Open(aContext.mId);
		links.Put(flashId, link);
		Database::link.Close(aContext.mId);
		link->Activate();
	}
}

static void WeaponOrdnance(EntityContext &aContext)
{
	// get parameters
	unsigned int ordnance(Expression::Read<unsigned int>(aContext));
	int track(Expression::Read<unsigned int>(aContext));
	Transform2 position(Cast<Transform2, __m128>(Expression::Evaluate<__m128>(aContext)));
	position.a *= float(M_PI) / 180.0f;
	Transform2 velocity(Cast<Transform2, __m128>(Expression::Evaluate<__m128>(aContext)));
	velocity.a *= float(M_PI) / 180.0f;

	// get the entity
	Entity *entity = Database::entity.Get(aContext.mId);

	// interpolated transform
	Transform2 basetransform(entity->GetInterpolatedTransform(aContext.mParam / sim_step));

	// get world position
	position *= basetransform;

	// get world velocity
	velocity.p = position.Rotate(velocity.p);

	// instantiate a bullet
	if (unsigned int ordId = Database::Instantiate(ordnance, Database::owner.Get(aContext.mId), aContext.mId, position.a, position.p, velocity.p, velocity.a))
	{
#ifdef DEBUG_WEAPON_CREATE_ORDNANCE
		DebugPrint("ordnance=\"%s\" owner=\"%s\"\n",
			Database::name.Get(ordId).c_str(),
			Database::name.Get(Database::owner.Get(ordId)).c_str());
#endif

		// set fractional turn
		if (Renderable *renderable = Database::renderable.Get(ordId))
			renderable->SetFraction(aContext.mParam / sim_step);

		// if tracking....
		if (track)
		{
			// add a tracker
			Database::weapontracker.Put(ordId, WeaponTracker(aContext.mId));
		}
	}
}

static void WeaponSound(EntityContext &aContext)
{
	unsigned int name(Expression::Read<unsigned int>(aContext));
	PlaySoundCue(aContext.mId, name);
}

void WeaponStartRepeat(EntityContext &aContext)
{
	// initialize repeat count and sequence offset
	unsigned int name(Expression::Read<unsigned int>(aContext));
	aContext.mVars->Put(name, Expression::Evaluate<float>(aContext));
	aContext.mVars->Put(name+1, Cast<float, unsigned int>(0));
}

void WeaponRepeat(EntityContext &aContext)
{
	const unsigned int *reset = aContext.mStream - 1;

	// get repeat count and sequence offset
	unsigned int name(Expression::Read<unsigned int>(aContext));
	float count = aContext.mVars->Get(name);
	unsigned int offset = Cast<unsigned int, float>(aContext.mVars->Get(name+1));

	// get repeat block
	size_t size(Expression::Read<size_t>(aContext));
	const unsigned int *begin = aContext.mStream;
	const unsigned int *end = aContext.mStream + size;

	// resume from saved offset
	aContext.mStream += offset;

	// while iterations left...
	while (count > 0.0f)
	{
		// evaluate the next expression
		Expression::Evaluate<void>(aContext);

		// if reaching the end...
		if (aContext.mStream >= end)
		{
			// rewind and decrement the count
			aContext.mStream = begin;
			count -= 1;
			if (count <= 0.0f)
				break;
		}

		// if out of time...
		if (aContext.mParam < -0.0001f)
		{
			// save repeat count and sequence offset
			aContext.mVars->Put(name, count);
			aContext.mVars->Put(name+1, Cast<float, unsigned int>(aContext.mStream - begin));
			aContext.mStream = reset;
			return;
		}
	}

	// clear out variables
	aContext.mVars->Delete(name);
	aContext.mVars->Delete(name+1);

	// jump to the end
	aContext.mStream = end;
}

// configue a parameter
static void ConfigureParameter(const TiXmlElement *element, const char *param, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// get constant value from attribute
	float value = defaults[0];
	element->QueryFloatAttribute(param, &value);

	// if there is a child tag for the parameter...
	if (const TiXmlElement *child = element->FirstChildElement(param))
	{
		// configure the expression
		ConfigureExpressionRoot<float>(child, buffer, names, &value);
	}
	else
	{
		// append a constant expression
		Expression::Append(buffer, Expression::Constant<float>, value);
	}
}

bool WeaponTemplate::ConfigureAction(const TiXmlElement *element, unsigned int aId)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x892e4ca0 /* "wait" */:
			{
				Expression::Append(mAction, WeaponWait);
				ConfigureExpressionRoot<float>(child, mAction, sScalarNames, sScalarDefault);
			}
			break;

		case 0xc4642eff /* "action" */:
			{
				// reserve size
				mAction.push_back(0);
				int start = mAction.size();

				// configure actions
				ConfigureAction(child, aId);

				// set size
				mAction[start-1] = mAction.size() - start;
			}
			break;

		case 0xd99ba82a /* "repeat" */:
			{
				// generate an identifier based on offset
				size_t id = mAction.size() * 4;

				// configure loop setup
				Expression::Append(mAction, WeaponStartRepeat, id);
				ConfigureParameter(child, "count", mAction, sScalarNames, sScalarDefault);

				// append loop repeat
				Expression::Append(mAction, WeaponRepeat, id);

				// configure loop body
				mAction.push_back(0);
				int start = mAction.size();
				ConfigureAction(child, aId);
				mAction[start-1] = mAction.size() - start;
			}
			break;

		case 0x63734e77 /* "recoil" */:
			Expression::Append(mAction, WeaponRecoil, mRecoil);
			break;

		case 0xaf85ad29 /* "flash" */:
			Expression::Append(mAction, WeaponFlash, Hash(child->Attribute("name")));
			if (const TiXmlElement *param = child->FirstChildElement("position"))
				ConfigureExpressionRoot<__m128>(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Constant<__m128>, _mm_setzero_ps());
			break;

		case 0x399bf05d /* "ordnance" */:
			Expression::Append(mAction, WeaponOrdnance, Hash(child->Attribute("name")), mTrack);
			if (const TiXmlElement *param = child->FirstChildElement("position"))
				ConfigureExpressionRoot<__m128>(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Constant<__m128>, _mm_setzero_ps());
			if (const TiXmlElement *param = child->FirstChildElement("velocity"))
				ConfigureExpressionRoot<__m128>(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Constant<__m128>, _mm_setzero_ps());
			break;

		case 0xe5561300 /* "cue" */:
			Expression::Append(mAction, WeaponSound, Hash(child->Attribute("name")));
			break;

#if 0
		case 0xd99ba82a /* "repeat" */:
			{
				int count = 1;
				child->QueryIntAttribute("count", &count);

				Expression::Append(mAction, Expression::Repeat, count);

				mAction.push_back(0);
				int start = mAction.size();
				ConfigureAction(child, aId);
				mAction[start-1] = mAction.size() - start;
			}
			break;
#endif

		case 0xddef486b /* "loop" */:
			{
				unsigned int name = Hash(child->Attribute("name"));
				float from = 0.0f;
				child->QueryFloatAttribute("from", &from);
				float to = 0.0f;
				child->QueryFloatAttribute("to", &to);
				float by = from < to ? 1.0f : -1.0f;
				child->QueryFloatAttribute("by", &by);

				if ((to - from) * by <= 0)
				{
					DebugPrint("loop name=\"%s\" from=\"%f\" to=\"%f\" by=\"%f\" would never terminate\n");
					break;
				}

				Expression::Append(mAction, Expression::Loop, name);
				Expression::Append(mAction, from, to, by);

				mAction.push_back(0);
				int start = mAction.size();
				ConfigureAction(child, aId);
				mAction[start-1] = mAction.size() - start;
			}
			break;
		}
	}

	return true;
}

bool WeaponTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x383251f6 /* "aim" */:
			{
				element->QueryFloatAttribute("x", &mAim.x);
				element->QueryFloatAttribute("y", &mAim.y);
			}
			break;

		case 0x63734e77 /* "recoil" */:
			{
				child->QueryFloatAttribute("value", &mRecoil);
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

		case 0x75413203 /* "trigger" */:
			{
				// TO DO: support single/automatic/charge
				switch (Hash(child->Attribute("type")))
				{
				case 0xadc649b8 /* "hold" */:		mTrigger = TRIGGER_HOLD; break;
				case 0x01fcf9b4 /* "press" */:		mTrigger = TRIGGER_PRESS; break;
				case 0x1036ae7e /* "release" */:	mTrigger = TRIGGER_RELEASE; break;
				case 0x316c9fa1 /* "invert" */:		mTrigger = TRIGGER_INVERT; break;
				case 0x6736afe4 /* "always" */:		mTrigger = TRIGGER_ALWAYS; break;
				};
				if (child->QueryIntAttribute("channel", &mChannel) == TIXML_SUCCESS)
					--mChannel;
			}
			break;

		case 0x5b9b0daf /* "ammo" */:
			{
				if (const char *type = child->Attribute("type"))
					mType = Hash(type);
				child->QueryFloatAttribute("cost", &mCost);
			}
			break;

		case 0xc4642eff /* "action" */:
			ConfigureAction(child, aId);
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
, mIndex(0)
, mTimer(0.0f)
, mLocal(0.0f)
, mPhase(0)
, mAmmo(0)
{
}

Weapon::Weapon(const WeaponTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mControlId(0)
, mChannel(aTemplate.mChannel)
, mPrevFire(0.0f)
, mTrack(0)
, mIndex(aTemplate.mAction.size())
, mTimer(0.0f)
, mLocal(0.0f)
, mPhase(aTemplate.mPhase)
, mAmmo(0)
{
	// if the action is empty...
	if (aTemplate.mAction.empty())
	{
		// set to none
		DebugPrint("warning: weapon \"%s\" has no action\n", Database::name.Get(aId));
		SetAction(Action(this, &Weapon::UpdateNone));
	}
	else
	{
		// set to ready
		SetAction(Action(this, &Weapon::UpdateReady));
	}

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

// weapon none update
void Weapon::UpdateNone(float aStep)
{
}

// weapon ready update
void Weapon::UpdateReady(float aStep)
{
	// get controller
	const Controller *controller = Database::controller.Get(mControlId);
	if (!controller)
		return;

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
	case WeaponTemplate::TRIGGER_INVERT: trigger = fire == 0; break;
	case WeaponTemplate::TRIGGER_ALWAYS: trigger = true; break;
	}
	mPrevFire = fire;

	// if triggered...
	if (trigger)
	{
		// rewind main timer
		mTimer -= weapon.mDelay / weapon.mCycle;

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

				// set action index
				mIndex = 0;

				// set local timer
				mLocal = -aStep;

				// switch to action
				SetAction(Action(this, &Weapon::UpdateAction));
				UpdateAction(aStep);
			}
			else
			{
				// start "empty" sound cue
				PlaySoundCue(mId, 0x18a7beee /* "empty" */);

				// switch to delay
				SetAction(Action(this, &Weapon::UpdateDelay));
				UpdateDelay(aStep);
			}

			// wrap around
			mPhase = weapon.mCycle - 1;
		}
		else
		{
			// advance phase
			--mPhase;

			// switch to delay
			SetAction(Action(this, &Weapon::UpdateDelay));
			UpdateDelay(aStep);
		}
	}
}

void Weapon::UpdateDelay(float aStep)
{
	// advance fire timer
	mTimer += aStep;

	// if the timer elapses...
	if (mTimer > -0.001f)
	{
		// advance to next event
		aStep -= mTimer;
		mTimer = 0.0f;

		// switch to ready
		SetAction(Action(this, &Weapon::UpdateReady));
		UpdateReady(aStep);
	}
}

void Weapon::UpdateAction(float aStep)
{
	// advance the local timer
	mLocal += aStep;

	// update fire timer
	mTimer += aStep;
	if (mTimer > 0.0f)
		mTimer = 0.0f;

	// if still waiting...
	if (mLocal < -0.001f)
	{
		// done
		return;
	}

	// get template data
	const WeaponTemplate &weapon = Database::weapontemplate.Get(mId);

	// create an entity context
	EntityContext context(&weapon.mAction.front(), weapon.mAction.size(), mLocal, mId);
	context.mStream += mIndex;

	// evaluate actions
	while (context.mParam > -0.001f && context.mStream < context.mEnd)
		Expression::Evaluate<void>(context);

	// read updated context
	mIndex = context.mStream - context.mBegin;
	mLocal = context.mParam;

	// if the action is completed or out of track slots...
	if (context.mStream >= context.mEnd || weapon.mTrack && mTrack >= weapon.mTrack)
	{
		// update fire timer
		mTimer += mLocal;

		if (mTimer > -aStep)
		{
			// advance to next event
			aStep = -mTimer;
			mTimer = 0.0f;

			// switch to ready
			SetAction(Action(this, &Weapon::UpdateReady));
			UpdateReady(aStep);
		}
		else
		{
			// switch to delay
			SetAction(Action(this, &Weapon::UpdateDelay));
		}
	}
}
