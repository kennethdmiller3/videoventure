#include "StdAfx.h"
#include "Ship.h"
#include "Entity.h"
#include "Collidable.h"
#include "Controller.h"
#include "Sound.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

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
		}
	}

	return true;
}


// Ship Default Constructor
Ship::Ship(void)
: Simulatable(0)
{
}

// Ship Constructor
Ship::Ship(const ShipTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
{
	// start the idle sound
	PlaySound(aId, 0xc301cf93 /* "idle" */);

	// also start a thrust sound
	PlaySound(aId, 0xd0624e33 /* "thrust" */);
}

// Ship Destructor
Ship::~Ship(void)
{
}

// configure
bool Ship::Configure(const TiXmlElement *element)
{
	return Simulatable::Configure(element);
}

// Ship Simulate
void Ship::Simulate(float aStep)
{
	// get entity
	Entity *entity = Database::entity.Get(mId);

	// get ship template
	const ShipTemplate &ship = Database::shiptemplate.Get(mId);

	// get ship collidable
	const Collidable *collidable = Database::collidable.Get(mId);
	if (!collidable)
		return;
	b2Body *body = collidable->GetBody();
	if (!body)
		return;

	// get ship controller
	const Controller *controller = Database::controller.Get(mId);
	if (!controller)
		return;

	// apply thrust
	{
		const Vector2 &mMove = controller->mMove;
		float control = std::min(mMove.LengthSq(), 1.0f);
		float acc = Lerp(ship.mMinAccel, ship.mMaxAccel, control);
		Matrix2 transform(entity->GetTransform());
		Vector2 localmove(transform.Unrotate(mMove));
		Vector2 localvel(
			localmove.x * ship.mStrafeVeloc,
			(localmove.y >= 0.0f)
			? Lerp(ship.mNeutralVeloc, ship.mForwardVeloc, localmove.y)
			: Lerp(ship.mNeutralVeloc, ship.mReverseVeloc, -localmove.y));
		Vector2 dv(transform.Rotate(localvel) - entity->GetVelocity());
		float it = std::min(acc * InvSqrt(dv.LengthSq() + 0.0001f), 1.0f / aStep);
		Vector2 new_thrust(dv * it * body->GetMass());
		body->ApplyForce(b2Vec2(new_thrust.x, new_thrust.y), body->GetXForm().position);

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
		const Vector2 &mAim = controller->mAim;
		float control = std::min(16.0f * mAim.LengthSq(), 1.0f);
		float aim_angle = -atan2f(mAim.x, mAim.y) - entity->GetAngle();
		if (aim_angle > float(M_PI))
			aim_angle -= 2.0f*float(M_PI);
		else if (aim_angle < -float(M_PI))
			aim_angle += 2.0f*float(M_PI);
		float new_omega = std::min(std::max(aim_angle / aStep, -ship.mMaxOmega * control), ship.mMaxOmega * control);
		body->SetAngularVelocity(new_omega);
	}
}
