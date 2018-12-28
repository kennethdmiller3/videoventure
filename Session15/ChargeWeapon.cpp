#include "StdAfx.h"
#include "ChargeWeapon.h"
#include "Entity.h"
#include "Controller.h"
#include "Link.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Sound.h"
#include "Resource.h"
#include "Interpolator.h"
#include "Variable.h"

#include "ExpressionConfigure.h"
#include "ExpressionAction.h"

#ifdef USE_POOL_ALLOCATOR
// chargeweapon pool
static MemoryPool sPool(sizeof(ChargeWeapon));
void *ChargeWeapon::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void ChargeWeapon::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif

namespace Database
{
	Typed<Typed<ChargeStateTemplate> > chargestatetemplate(0x14776404 /* "chargestatetemplate" */);
	Typed<ChargeWeaponTemplate> chargeweapontemplate(0x272fa7f3 /* "chargeweapontemplate" */);
	Typed<ChargeWeapon *> chargeweapon(0xd416836d /* "chargeweapon" */);

	namespace Loader
	{
		static void ChargeWeaponConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			ChargeWeaponTemplate &chargeweapon = Database::chargeweapontemplate.Open(aId);
			chargeweapon.Configure(element, aId);
			Database::chargeweapontemplate.Close(aId);
		}
		Configure chargeweaponconfigure(0xd416836d /* "chargeweapon" */, ChargeWeaponConfigure);
	}

	namespace Initializer
	{
		static void ChargeWeaponActivate(unsigned int aId)
		{
			const ChargeWeaponTemplate &chargeweapontemplate = Database::chargeweapontemplate.Get(aId);
			ChargeWeapon *chargeweapon = new ChargeWeapon(chargeweapontemplate, aId);
			Database::chargeweapon.Put(aId, chargeweapon);
			chargeweapon->SetControl(aId);
		}
		Activate chargeweaponactivate(0x272fa7f3 /* "chargeweapontemplate" */, ChargeWeaponActivate);

		static void ChargeWeaponPostActivate(unsigned int aId)
		{
			ChargeWeapon *chargeweapon = Database::chargeweapon.Get(aId);
			for (unsigned int aControlId = aId; aControlId != 0; aControlId = Database::backlink.Get(aControlId))
			{
				if (const Controller *controller = Database::controller.Get(aControlId))
				{
					chargeweapon->SetControl(aControlId);
					break;
				}
			}
			chargeweapon->Activate();
		}
		PostActivate chargeweaponpostactivate(0x272fa7f3 /* "chargeweapontemplate" */, ChargeWeaponPostActivate);

		static void ChargeWeaponDeactivate(unsigned int aId)
		{
			if (ChargeWeapon *chargeweapon = Database::chargeweapon.Get(aId))
			{
				delete chargeweapon;
				Database::chargeweapon.Delete(aId);
			}
		}
		Deactivate chargeweapondeactivate(0x272fa7f3 /* "chargeweapontemplate" */, ChargeWeaponDeactivate);
	}
}

// actions

static const char * const sTransformNames[] = { "x", "y", "angle", ""};
static const float sTransformDefault[] = { 0.0f, 0.0f, 0.0f, 0.0f };

template<> inline Transform2 Cast<Transform2, __m128>(__m128 i)
{
	return Transform2(i.m128_f32[2], Vector2(i.m128_f32[0], i.m128_f32[1]));
}

template<> inline __m128 Cast<__m128, Transform2>(Transform2 i)
{
	return _mm_setr_ps(i.p.x, i.p.y, i.a, 0);
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
			if (CollidableBody *body = Database::collidablebody.Get(id))
			{
				Collidable::ApplyImpulse(body, transform.Rotate(Vector2(0, -recoil)));
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
	unsigned int size(Expression::Read<unsigned int>(aContext));
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
			aContext.mVars->Put(name+1, Cast<float, unsigned int>(unsigned int(aContext.mStream - begin)));
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

// get velocity in entity-local space
__m128 EvaluateVelocityLocal(EntityContext &aContext)
{
	if (const Entity *entity = Database::entity.Get(aContext.mId))
	{
		const Transform2 transform = entity->GetInterpolatedTransform(sim_fraction);
		const Vector2 velocity(transform.Unrotate(entity->GetVelocity()));
		const float omega(entity->GetOmega());
		return _mm_setr_ps(velocity.x, velocity.y, omega, 0.0f);
	}
	else
	{
		return _mm_setzero_ps();
	}
}

// configue a parameter
static void ConfigureParameter(const tinyxml2::XMLElement *element, const char *param, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// get constant value from attribute
	float value = defaults[0];
	element->QueryFloatAttribute(param, &value);

	// if there is a child tag for the parameter...
	if (const tinyxml2::XMLElement *child = element->FirstChildElement(param))
	{
		// configure the expression
		Expression::Loader<float>::ConfigureRoot(child, buffer, names, &value);
	}
	else
	{
		// append a constant expression
		Expression::Append(buffer, Expression::Read<float>, value);
	}
}


ChargeStateTemplate::ChargeStateTemplate(void)
: mNext(0)
, mTime(0.0f)
, mCost(0.0f)
{
}

ChargeStateTemplate::~ChargeStateTemplate(void)
{
}


bool ChargeStateTemplate::ConfigureAction(const tinyxml2::XMLElement *element, unsigned int aId)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x892e4ca0 /* "wait" */:
			{
				Expression::Append(mAction, WeaponWait);
				Expression::Loader<float>::ConfigureRoot(child, mAction, sScalarNames, sScalarDefault);
			}
			break;

		case 0xc4642eff /* "action" */:
			{
				// reserve size
				size_t buffer_size_at = mAction.size();
				Expression::Alloc(mAction, sizeof(unsigned int));
				size_t start = mAction.size();

				// configure actions
				ConfigureAction(child, aId);

				// set size
				*new (mAction.data() + buffer_size_at) unsigned int = unsigned int(mAction.size() - start);
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
				size_t buffer_size_at = mAction.size();
				Expression::Alloc(mAction, sizeof(unsigned int));
				size_t start = mAction.size();
				ConfigureAction(child, aId);
				*new (mAction.data() + buffer_size_at) unsigned int = unsigned int(mAction.size() - start);
			}
			break;

		case 0x63734e77 /* "recoil" */:
			Expression::Append(mAction, WeaponRecoil);
			Expression::Loader<float>::ConfigureRoot(child, mAction, sScalarNames, sScalarDefault);
			break;

		case 0xaf85ad29 /* "flash" */:
			Expression::Append(mAction, WeaponFlash, Hash(child->Attribute("name")));
			if (const tinyxml2::XMLElement *param = child->FirstChildElement("position"))
				Expression::Loader<__m128>::ConfigureRoot(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Read<__m128>, _mm_setzero_ps());
			break;

		case 0x399bf05d /* "ordnance" */:
			Expression::Append(mAction, WeaponOrdnance, Hash(child->Attribute("name")));
			if (const tinyxml2::XMLElement *param = child->FirstChildElement("position"))
				Expression::Loader<__m128>::ConfigureRoot(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Read<__m128>, _mm_setzero_ps());
			if (const tinyxml2::XMLElement *param = child->FirstChildElement("velocity"))
				Expression::Loader<__m128>::ConfigureRoot(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Read<__m128>, _mm_setzero_ps());
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

				size_t buffer_size_at = mAction.size();
				Expression::Alloc(mAction, sizeof(unsigned int));
				size_t start = mAction.size();
				ConfigureAction(child, aId);
				*new (mAction.data() + buffer_size_at) unsigned int = unsigned int(mAction.size() - start);
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

				size_t buffer_size_at = mAction.size();
				Expression::Alloc(mAction, sizeof(unsigned int));
				size_t start = mAction.size();
				ConfigureAction(child, aId);
				*new (mAction.data() + buffer_size_at) unsigned int = unsigned int(mAction.size() - start);
			}
			break;
		}
	}

	return true;
}

bool ChargeStateTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	// clear any existing action
	// TO DO: support inheritance
	// TO DO: support "call"
	mAction.clear();

	// get state time
	element->QueryFloatAttribute("time", &mTime);

	// get next state
	if (const char *next = element->Attribute("next"))
		mNext = Hash(next);

	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x5b9b0daf /* "ammo" */:
			child->QueryFloatAttribute("cost", &mCost);
			break;

		case 0xc4642eff /* "action" */:
			ConfigureAction(child, aId);
			break;
		}
	}

	return true;
}

ChargeWeaponTemplate::ChargeWeaponTemplate(void)
: mChannel(0)
, mType(0)
, mStart(0)
{
}

ChargeWeaponTemplate::~ChargeWeaponTemplate(void)
{
}

bool ChargeWeaponTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x783132f6 /* "state" */:
			if (const char *name = child->Attribute("name"))
			{
				unsigned int aSubId = Hash(name);
				if (mStart == 0)
					mStart = aSubId;	// HACK
				Database::Typed<ChargeStateTemplate> &chargestates = Database::chargestatetemplate.Open(aId);
				ChargeStateTemplate &chargestate = chargestates.Open(aSubId);
				chargestate.Configure(child, aId);
				chargestates.Close(aSubId);
				Database::chargestatetemplate.Close(aId);
			}
			break;

		case 0x5b9b0daf /* "ammo" */:
			if (const char *type = child->Attribute("type"))
				mType = Hash(type);
			break;

		case 0x75413203 /* "trigger" */:
			if (child->QueryIntAttribute("channel", &mChannel) == tinyxml2::XML_SUCCESS)
				--mChannel;
			break;

		}
	}

	return true;
}


ChargeWeapon::ChargeWeapon(void)
: Updatable(0)
, mControlId(0)
, mChannel(0)
, mState(0)
, mTimer(0.0f)
, mLocal(0.0f)
, mAmmo(0)
{
}

ChargeWeapon::ChargeWeapon(const ChargeWeaponTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mControlId(0)
, mChannel(aTemplate.mChannel)
, mState(aTemplate.mStart)
, mTimer(0.0f)
, mLocal(0.0f)
, mAmmo(0)
{
	// set to ready
	SetAction(Action(this, &ChargeWeapon::UpdateReady));

	// if the chargeweapon uses ammo...
	if (aTemplate.mType)
	{
		// find the specified resource
		mAmmo = FindResourceContainer(aId, aTemplate.mType);
	}
}

ChargeWeapon::~ChargeWeapon(void)
{
}

// chargeweapon none update
void ChargeWeapon::UpdateNone(float aStep)
{
}

// chargeweapon ready update
void ChargeWeapon::UpdateReady(float aStep)
{
	// get controller
	const Controller *controller = Database::controller.Get(mControlId);
	if (!controller)
		return;

	// get trigger value
	float trigger = controller->mFire[mChannel];

	// if triggered...
	if (trigger)
	{
		// start timer
		mTimer = 0.0f;

		// switch to charge
		SetAction(Action(this, &ChargeWeapon::UpdateCharge));
		UpdateCharge(aStep);
	}
}

void ChargeWeapon::UpdateCharge(float aStep)
{
	// get controller
	const Controller *controller = Database::controller.Get(mControlId);
	if (!controller)
		return;

	// get trigger value
	float trigger = controller->mFire[mChannel];

	// if triggered...
	if (trigger)
	{
		// accumulate charge time
		mTimer += aStep;

		// get the current state
		const ChargeStateTemplate *chargestate = &Database::chargestatetemplate.Get(mId).Get(mState);

		// while there is time remaining...
		while (mTimer >= chargestate->mTime)
		{
			// if the state has no next state...
			if (chargestate->mNext == 0)
			{
				// clamp to state time
				DebugPrint("no next state\n");
				mTimer = chargestate->mTime;
				break;
			}

			// get the next state
			const ChargeStateTemplate *nextstate = &Database::chargestatetemplate.Get(mId).Get(chargestate->mNext);

			// if using ammo
			if (nextstate->mCost)
			{
				// get the weapon template
				const ChargeWeaponTemplate &chargeweapon = Database::chargeweapontemplate.Get(mId);

				// ammo resource (if any)
				Resource *resource = Database::resource.Get(mAmmo).Get(chargeweapon.mType);

				// if not enough ammo for the next state...
				if (resource && nextstate->mCost > resource->GetValue())
				{
					// clamp to state time
					DebugPrint("next cost %f > ammo %f\n", nextstate->mCost, resource->GetValue());
					mTimer = chargestate->mTime;
					break;
				}
			}

			// deduct time
			mTimer -= chargestate->mTime;

			// switch states
			mState = chargestate->mNext;
			chargestate = nextstate;
			DebugPrint("switch to %08x\n", mState);
		}
	}
	else
	{
		// rewind timer
		mTimer = 0.0f;

		// get the current state
		const ChargeStateTemplate &chargestate = Database::chargestatetemplate.Get(mId).Get(mState);

		// if using ammo
		if (chargestate.mCost)
		{
			// get the weapon template
			const ChargeWeaponTemplate &chargeweapon = Database::chargeweapontemplate.Get(mId);

			// ammo resource (if any)
			Resource *resource = Database::resource.Get(mAmmo).Get(chargeweapon.mType);

			// if not enough ammo to fire
			if (resource && chargestate.mCost > resource->GetValue())
			{
				DebugPrint("fire: cost %f > ammo %f\n", chargestate.mCost, resource->GetValue());

				// start "empty" sound cue
				PlaySoundCue(mId, 0x18a7beee /* "empty" */);

				// switch to ready
				SetAction(Action(this, &ChargeWeapon::UpdateReady));
				return;
			}

			// deduct ammo
			resource->Add(mId, -chargestate.mCost);
		}

		if (!chargestate.mAction.empty())
		{
			DebugPrint("fire: start\n");

			// set action index
			mIndex = 0;

			// set local timer
			mLocal = -aStep;

			// switch to action
			SetAction(Action(this, &ChargeWeapon::UpdateAction));
			UpdateAction(aStep);
		}
	}
}

void ChargeWeapon::UpdateDelay(float aStep)
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
		SetAction(Action(this, &ChargeWeapon::UpdateReady));
		UpdateReady(aStep);
	}
}

void ChargeWeapon::UpdateAction(float aStep)
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
	const ChargeStateTemplate &chargestate = Database::chargestatetemplate.Get(mId).Get(mState);

	// create an entity context
	EntityContext context(&chargestate.mAction.front(), chargestate.mAction.size(), mLocal, mId);
	context.mStream += mIndex;

	// evaluate actions
	while (context.mParam > -0.001f && context.mStream < context.mEnd)
		Expression::Evaluate<void>(context);

	// read updated context
	mIndex = context.mStream - context.mBegin;
	mLocal = context.mParam;

	// if the action is completed...
	if (context.mStream >= context.mEnd)
	{
		// update fire timer
		mTimer += mLocal;

		// get template data
		const ChargeWeaponTemplate &chargeweapon = Database::chargeweapontemplate.Get(mId);

		// reset to start state
		mState = chargeweapon.mStart;

		if (mTimer > -aStep)
		{
			// advance to next event
			aStep = -mTimer;
			mTimer = 0.0f;

			// switch to ready
			SetAction(Action(this, &ChargeWeapon::UpdateReady));
			UpdateReady(aStep);
		}
		else
		{
			// switch to delay
			SetAction(Action(this, &ChargeWeapon::UpdateDelay));
		}
	}
}
