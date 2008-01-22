#include "StdAfx.h"
#include "Ship.h"
#include "Entity.h"
#include "Collidable.h"
#include "Controller.h"


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
: mMaxVeloc(200)
, mMaxAccel(1000)
, mFriction(50)
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

	element->QueryFloatAttribute("maxveloc", &mMaxVeloc);
	element->QueryFloatAttribute("maxaccel", &mMaxAccel);
	element->QueryFloatAttribute("friction", &mFriction);
	element->QueryFloatAttribute("maxomega", &mMaxOmega);
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
	Entity *entity = Database::entity.Get(id);

	// get ship template
	const ShipTemplate &ship = Database::shiptemplate.Get(id);

	// get ship collidable
	const Collidable *collidable = Database::collidable.Get(id);
	b2Body *body = collidable->GetBody();

	// get ship controller
	const Controller *controller = Database::controller.Get(id);

	// apply thrust
	{
		const Vector2 &mMove = controller->mMove;
		float control = std::min(mMove.LengthSq(), 1.0f);
		float acc = ship.mFriction + (ship.mMaxAccel - ship.mFriction) * control;
		Vector2 dv(mMove * ship.mMaxVeloc - entity->GetVelocity());
		float it = std::min(acc * InvSqrt(dv.LengthSq() + 0.0001f), 1.0f / aStep);
		Vector2 new_thrust(dv * it * body->GetMass());
		body->ApplyForce(b2Vec2(new_thrust.x, new_thrust.y), body->GetXForm().position);
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
