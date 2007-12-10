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

		case 0x6f332041 /* "weapon" */:
			child->QueryIntAttribute("cycle", &mCycle);
			child->QueryIntAttribute("phase", &mPhase);
			break;
		}
	}

	return true;
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

	// if the owner does not exist...
	if (!Database::entity.Get(owner))
	{
		// self-destruct
		Database::Delete(Simulatable::id);
		return;
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
