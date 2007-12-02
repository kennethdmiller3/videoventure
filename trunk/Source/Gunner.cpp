#include "StdAfx.h"
#include "Gunner.h"
#include "Bullet.h"

// gunner bullet physics
const float GUNNER_BULLET_SPEED = 800;

// bullet direction (keyed by shot index)
const float GUNNER_BULLET_ANGLE = 5*(float)M_PI/180;
const Vector2 GUNNER_BULLET_DIR[2] =
{
	Vector2(sinf(-GUNNER_BULLET_ANGLE), cosf(-GUNNER_BULLET_ANGLE)),
	Vector2(sinf(GUNNER_BULLET_ANGLE), cosf(GUNNER_BULLET_ANGLE)),
};


// Gunner Constructor
Gunner::Gunner(unsigned int aId, unsigned int aParentId)
: Entity(aId)
, Controllable()
, Simulatable()
, Collidable(Database::collidabletemplate.Get(aParentId))
, Renderable(Database::renderabletemplate.Get(aParentId))
, player(NULL), offset(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0)), mDelay(0.0f), mPhase(-1), mCycle(0)
{
}

// Gunner Destructor
Gunner::~Gunner(void)
{
}

// configure
bool Gunner::Configure(TiXmlElement *element)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0xe063cbaa /* "gunner" */:
		{
			const char *owner = element->Attribute("owner");
			player = dynamic_cast<Player *>(Database::entity.Get(Hash(owner)));

			element->QueryIntAttribute("phase", &mPhase);
		}
		return true;

	case 0x14c8d3ca /* "offset" */:
		{
			element->QueryFloatAttribute("x", &offset.p.x);
			element->QueryFloatAttribute("y", &offset.p.y);
			float angle = 0.0f;
			if (element->QueryFloatAttribute("angle", &angle) == TIXML_SUCCESS)
			{
				angle *= float(M_PI)/180.0f;
				offset.x.x = cosf(angle);
				offset.x.y = sinf(angle);
				offset.y.x = -offset.x.y;
				offset.y.y = offset.x.x;
			}
		}
		return true;

	default:
		return Entity::Configure(element) || Controllable::Configure(element) || Simulatable::Configure(element) || Collidable::Configure(element) || Renderable::Configure(element);
	}
}

// Gunner Init
void Gunner::Init(void)
{
	// offset from player position
	Matrix2 transform = offset * player->GetTransform();
	body->SetCenterPosition(b2Vec2(transform.p.x, transform.p.y), -atan2f(transform.y.x, transform.y.y));

	// constrain to the offset position
	b2RevoluteJointDef joint;
	joint.body1 = player->GetBody();
	joint.body2 = GetBody();
	joint.anchorPoint = joint.body1->GetCenterPosition();
	joint.lowerAngle = -atan2f(offset.y.x, offset.y.y);
	joint.upperAngle = -atan2f(offset.y.x, offset.y.y);
	joint.enableLimit = true;
	joint.motorTorque = 1.0f;
	joint.motorSpeed = 0.0f;
    joint.enableMotor = true;
	world->CreateJoint(&joint);
}

// Gunner Control
void Gunner::Control(float aStep)
{
	if (!player)
		return;

	const Input *input = player->GetInput();

	if ((*input)[Input::FIRE_PRIMARY])
	{
		if (mDelay <= 0.0f)
		{
			if (mCycle == mPhase)
			{
				for (int i = 0; i < 2; i++)
				{
					const Vector2 d = GUNNER_BULLET_DIR[i];
					Bullet *bullet = Bullet::pool.construct(0, 0xd85669f0 /* "playerbullet" */);
					bullet->SetTransform(transform);
					bullet->SetVelocity(transform.Rotate(d) * GUNNER_BULLET_SPEED);
					bullet->AddToWorld();
				}
			}

			mDelay += 0.125f;
			mCycle = 1 - mCycle;
		}
	}
}

// Gunner Simulate
void Gunner::Simulate(float aStep)
{
	if (!player)
		return;

	// update fire delay
	mDelay -= aStep;
	if (mDelay < 0.0f)
		mDelay = 0.0f;
}

// Gunner Render
void Gunner::Render()
{
	// push a transform
	glPushMatrix();

	// load matrix
	float m[16] =
	{
		transform.y.y, -transform.y.x, 0, 0,
		transform.y.x, transform.y.y, 0, 0,
		0, 0, 1, 0,
		transform.p.x, transform.p.y, 0, 1
	};
	glMultMatrixf( m );

	// call draw list
	glCallList(mDraw);

	// reset the transform
	glPopMatrix();
}
