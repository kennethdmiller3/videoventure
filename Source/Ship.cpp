#include "StdAfx.h"
#include "Ship.h"
#include "Entity.h"
#include "Collidable.h"
#include "Controller.h"
#include "Sound.h"
#include "Variable.h"


#ifdef USE_POOL_ALLOCATOR
// ship pool
static MemoryPool sPool(sizeof(Ship));
void *Ship::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Ship::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<ShipTemplate> shiptemplate(0xf71a421d /* "shiptemplate" */);
	Typed<Ship *> ship(0xac56f17f /* "ship" */);

	namespace Loader
	{
		static void ShipConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			ShipTemplate &ship = Database::shiptemplate.Open(aId);
			ship.Configure(element);
			Database::shiptemplate.Close(aId);
		}
		Configure shipconfigure(0xac56f17f /* "ship" */, ShipConfigure);
	}

	namespace Initializer
	{
		static void ShipActivate(unsigned int aId)
		{
			const ShipTemplate &shiptemplate = Database::shiptemplate.Get(aId);
			Ship *ship = new Ship(shiptemplate, aId);
			Database::ship.Put(aId, ship);
			ship->Activate();
		}
		Activate shipactivate(0xf71a421d /* "shiptemplate" */, ShipActivate);

		static void ShipDeactivate(unsigned int aId)
		{
			if (Ship *ship = Database::ship.Get(aId))
			{
				delete ship;
				Database::ship.Delete(aId);
			}
		}
		Deactivate shipdeactivate(0xf71a421d /* "shiptemplate" */, ShipDeactivate);
	}
}

// Ship Template Constructor
ShipTemplate::ShipTemplate(void)
: mReverseVeloc(-200)
, mNeutralVeloc(0)
, mForwardVeloc(200)
, mStrafeVeloc(200)
, mMinAccel(50)
, mMaxAccel(1000)
, mMaxOmega(10)
{
}

// Ship Template Destructor
ShipTemplate::~ShipTemplate(void)
{
}

// Ship Template Configure
bool ShipTemplate::Configure(const tinyxml2::XMLElement *element)
{
	if (Hash(element->Value()) != 0xac56f17f /* "ship" */)
		return false;

	float mMaxVeloc;
	if (element->QueryFloatAttribute("maxveloc", &mMaxVeloc) == tinyxml2::XML_SUCCESS)
	{
		mReverseVeloc = -mMaxVeloc;
		mNeutralVeloc = 0.0f;
		mForwardVeloc = mMaxVeloc;
		mStrafeVeloc = mMaxVeloc;
	}
	element->QueryFloatAttribute("forwardveloc", &mForwardVeloc);
	element->QueryFloatAttribute("neutralveloc", &mNeutralVeloc);
	element->QueryFloatAttribute("reverseveloc", &mReverseVeloc);
	element->QueryFloatAttribute("strafeveloc", &mStrafeVeloc);
	element->QueryFloatAttribute("maxaccel", &mMaxAccel);
	element->QueryFloatAttribute("minaccel", &mMinAccel);
	element->QueryFloatAttribute("maxomega", &mMaxOmega);

	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x0e0d9594 /* "sound" */:
			Database::Loader::Configure::Get(0x0e0d9594 /* "sound" */);

		case 0xde1ed562 /* "veloc" */:
			element->QueryFloatAttribute("forward", &mForwardVeloc);
			element->QueryFloatAttribute("neutral", &mNeutralVeloc);
			element->QueryFloatAttribute("reverse", &mReverseVeloc);
			element->QueryFloatAttribute("strafe", &mStrafeVeloc);
			break;

		case 0x4db6f71f /* "maxaccel" */:
			element->QueryFloatAttribute("value", &mMaxAccel);
			break;

		case 0xe03a69d9 /* "minaccel" */:
			element->QueryFloatAttribute("value", &mMinAccel);
			break;
		}
	}

	return true;
}


// Ship Default Constructor
Ship::Ship(void)
: Simulatable(0)
{
	SetAction(Action(this, &Ship::Simulate));
}

// Ship Constructor
Ship::Ship(const ShipTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
{
	SetAction(Action(this, &Ship::Simulate));

	// start the idle sound
	PlaySoundCue(aId, 0xc301cf93 /* "idle" */);

	// also start a thrust sound
	PlaySoundCue(aId, 0xd0624e33 /* "thrust" */);
}

// Ship Destructor
Ship::~Ship(void)
{
}

// Ship Simulate
void Ship::Simulate(float aStep)
{
	// get entity
	Entity *entity = Database::entity.Get(mId);

	// get ship template
	const ShipTemplate &ship = Database::shiptemplate.Get(mId);

	// get collision body (if any)
	CollidableBody *body = Database::collidablebody.Get(mId);

	// get ship controller
	const Controller *controller = Database::controller.Get(mId);

	// apply thrust
	{
		Vector2 mMove = controller ? controller->mMove : Vector2(0, 0);
		float control = mMove.LengthSq();
		if (control > 1.0f)
		{
			control = 1.0f;
			mMove *= InvSqrt(control);
		}
		float acc = Lerp(ship.mMinAccel, ship.mMaxAccel, control);
		Vector2 localvel(
			mMove.x * ship.mStrafeVeloc,
			(mMove.y >= 0.0f)
			? Lerp(ship.mNeutralVeloc, ship.mForwardVeloc, mMove.y)
			: Lerp(ship.mNeutralVeloc, ship.mReverseVeloc, -mMove.y));
		const Transform2 &transform = entity->GetTransform();
		Vector2 dv(transform.Rotate(localvel) - entity->GetVelocity());
		float scale = std::min(aStep * acc * InvSqrt(dv.LengthSq() + 0.0001f), 1.0f);
		dv *= scale;
		if (body)
			Collidable::AddVelocity(body, dv);
		else
			entity->SetVelocity(entity->GetVelocity() + dv);

		// save throttle for the renderer
		Database::Typed<float> &variables = Database::variable.Open(mId);
		variables.Put(0xd0624e33 /* "thrust" */ + 0, mMove.x);
		variables.Put(0xd0624e33 /* "thrust" */ + 1, mMove.y);
		variables.Put(0xd0624e33 /* "thrust" */ + 2, 0.0f);
		variables.Put(0xd0624e33 /* "thrust" */ + 3, 1.0f);
		Database::variable.Close(mId);

		// update thrust sound (if any)
		Database::Typed<Sound *> &sounds = Database::sound.Open(mId);
		if (Sound *s = sounds.Get(0xd0624e33 /* "thrust" */))
		{
			const unsigned int &soundid = Database::soundcue.Get(mId).Get(0xd0624e33 /* "thrust" */);
			const SoundTemplate &soundtemplate = Database::soundtemplate.Get(soundid);
			s->mVolume = control * soundtemplate.mVolume;
		}
	}

	// apply steering
	{
		const float mTurn = controller ? controller->mTurn : 0.0f;
		if (body)
			Collidable::SetOmega(body, mTurn * ship.mMaxOmega);
		else
			entity->SetOmega(mTurn * ship.mMaxOmega);
	}
}
