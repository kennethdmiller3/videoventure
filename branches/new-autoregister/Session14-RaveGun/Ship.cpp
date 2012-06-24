#include "StdAfx.h"
#include "Ship.h"
#include "Entity.h"
#include "Collidable.h"
#include "Controller.h"
#include "Sound.h"
#include "Drawlist.h"


#ifdef USE_POOL_ALLOCATOR
// ship pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Ship));
void *Ship::operator new(size_t aSize)
{
	return pool.malloc();
}
void Ship::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<ShipTemplate> shiptemplate(0xf71a421d /* "shiptemplate" */);
	Typed<Ship *> ship(0xac56f17f /* "ship" */);

	namespace Loader
	{
		class ShipLoader
		{
		public:
			ShipLoader()
			{
				AddConfigure(0xac56f17f /* "ship" */, Entry(this, &ShipLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				ShipTemplate &ship = Database::shiptemplate.Open(aId);
				ship.Configure(element);
				Database::shiptemplate.Close(aId);
			}
		}
		shiploader;
	}

	namespace Initializer
	{
		class ShipInitializer
		{
		public:
			ShipInitializer()
			{
				AddActivate(0xf71a421d /* "shiptemplate" */, Entry(this, &ShipInitializer::Activate));
				AddDeactivate(0xf71a421d /* "shiptemplate" */, Entry(this, &ShipInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const ShipTemplate &shiptemplate = Database::shiptemplate.Get(aId);
				Ship *ship = new Ship(shiptemplate, aId);
				Database::ship.Put(aId, ship);
				ship->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Ship *ship = Database::ship.Get(aId))
				{
					delete ship;
					Database::ship.Delete(aId);
				}
			}
		}
		shipinitializer;
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
bool ShipTemplate::Configure(const TiXmlElement *element)
{
	if (Hash(element->Value()) != 0xac56f17f /* "ship" */)
		return false;

	float mMaxVeloc;
	if (element->QueryFloatAttribute("maxveloc", &mMaxVeloc) == TIXML_SUCCESS)
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

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x0e0d9594 /* "sound" */:
			Database::Loader::GetConfigure(0x0e0d9594 /* "sound" */);

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
	b2Body *body = NULL;
	if (const Collidable *collidable = Database::collidable.Get(mId))
		body = collidable->GetBody();

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
		float it = std::min(acc * InvSqrt(dv.LengthSq() + 0.0001f), 1.0f / aStep);
		Vector2 new_accel(dv * it);
		if (body)
			body->ApplyForce(new_accel * body->GetMass(), body->GetTransform().position);
		else
			entity->SetVelocity(entity->GetVelocity() + aStep * new_accel);

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
			body->SetAngularVelocity(mTurn * ship.mMaxOmega);
		else
			entity->SetOmega(mTurn * ship.mMaxOmega);
	}
}
