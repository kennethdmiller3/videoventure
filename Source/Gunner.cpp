#include "StdAfx.h"
#include "Gunner.h"
#include "Bullet.h"

// gunner bullet physics
const float GUNNER_BULLET_SPEED = 800;

// bullet direction (keyed by shot index)
const float GUNNER_BULLET_ANGLE = 5*(float)M_PI/180;
const float GUNNER_BULLET_DIR[2] = { -GUNNER_BULLET_ANGLE, GUNNER_BULLET_ANGLE };

namespace Database
{
	Typed<Gunner *> gunner("gunner");
}

// Gunner Constructor
Gunner::Gunner(unsigned int aId, unsigned int aParentId)
: Controllable(aId)
, Simulatable(aId)
, owner(0)
, offset(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0))
, mFire(false)
, mDelay(0.0f)
, mPhase(0)
, mCycle(1)
{
}

// Gunner Destructor
Gunner::~Gunner(void)
{
}

// configure
bool Gunner::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0xe063cbaa /* "gunner" */)
		return false;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xf5674cd4 /* "owner" */:
			owner = Hash(child->Attribute("name"));
			break;

		case 0x14c8d3ca /* "offset" */:
			{
				child->QueryFloatAttribute("x", &offset.p.x);
				child->QueryFloatAttribute("y", &offset.p.y);
				float angle = 0.0f;
				if (child->QueryFloatAttribute("angle", &angle) == TIXML_SUCCESS)
					offset = Matrix2(angle * float(M_PI) / 180.0f, offset.p);
			}
			break;

		case 0x6f332041 /* "weapon" */:
			child->QueryIntAttribute("cycle", &mCycle);
			child->QueryIntAttribute("phase", &mPhase);
			break;
		}
	}

	return true;
}

// Gunner Init
void Gunner::Init(void)
{
	// offset from player position
	Matrix2 transform = offset * Database::entity.Get(owner)->GetTransform();
	Database::entity.Get(Simulatable::id)->SetTransform(transform);

	// update collidable body
	const Collidable *player_collidable = Database::collidable.Get(owner);
	const Collidable *gunner_collidable = Database::collidable.Get(Simulatable::id);
	if (player_collidable->GetBody() && gunner_collidable->GetBody())
	{
		gunner_collidable->GetBody()->SetCenterPosition(transform.p, transform.Angle());

		// constrain to the offset position
		b2PrismaticJointDef joint;
		joint.body1 = player_collidable->GetBody();
		joint.body2 = gunner_collidable->GetBody();
		joint.anchorPoint = joint.body1->GetCenterPosition();
		joint.enableLimit = true;
		joint.motorForce = 100.0f;
		joint.motorSpeed = 0.0f;
		joint.enableMotor = true;
		Collidable::GetWorld()->CreateJoint(&joint);
	}
}

// Gunner Control
void Gunner::Control(float aStep)
{
	if (!owner)
		return;

	// get fire control
	mFire = input[Input::FIRE_PRIMARY] != 0.0f;
}

// Gunner Simulate
void Gunner::Simulate(float aStep)
{
	if (!owner)
		return;

	// update collidable body
	const Collidable *player_collidable = Database::collidable.Get(owner);
	const Collidable *gunner_collidable = Database::collidable.Get(Simulatable::id);
	if (player_collidable->GetBody() && gunner_collidable->GetBody())
	{
		// cancel velocity offset
		// (prevents "wiggle" in the joint constraint)
		b2Body *player_body = player_collidable->GetBody();
		b2Body *gunner_body = gunner_collidable->GetBody();
		b2Vec2 dv((player_body->GetLinearVelocity() - gunner_body->GetLinearVelocity() - b2Cross(player_body->GetAngularVelocity(), player_body->GetCenterPosition() - gunner_body->GetCenterPosition())));
		gunner_body->ApplyImpulse(gunner_body->GetMass() * dv, gunner_body->GetCenterPosition());
	}

	// advance fire timer
	mDelay -= aStep * mCycle;

	// if ready to fire...
	if (mDelay <= 0.0f)
	{
		// if triggered...
		if (mFire)
		{
			// if firing on this phase...
			if (mPhase == 0)
			{
				// get the entity
				Entity *entity = Database::entity.Get(Simulatable::id);

				// for each shot...
				for (int i = 0; i < 2; i++)
				{
					// generate a bullet
					const float d = GUNNER_BULLET_DIR[i];
					Matrix2 transform(entity->GetAngle() + d, entity->GetPosition());
					Database::Instantiate(0xd85669f0 /* "playerbullet" */,
						entity->GetAngle() + d, entity->GetPosition(), transform.y * GUNNER_BULLET_SPEED);
				}

				// wrap around
				mPhase = mCycle - 1;
			}
			else
			{
				// advance phase
				--mPhase;
			}

			// update weapon delay
			mDelay += 0.2f;
		}
		else
		{
			// clamp fire delay
			mDelay = 0.0f;
		}
	}
}
