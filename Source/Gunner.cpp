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
, player(NULL), offset(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0)), mDelay(0.0f), mPhase(0), mCycle(1)
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

			element->QueryIntAttribute("cycle", &mCycle);
			element->QueryIntAttribute("phase", &mPhase);
		}
		return true;

	case 0x14c8d3ca /* "offset" */:
		{
			element->QueryFloatAttribute("x", &offset.p.x);
			element->QueryFloatAttribute("y", &offset.p.y);
			float angle = 0.0f;
			if (element->QueryFloatAttribute("angle", &angle) == TIXML_SUCCESS)
				offset = Matrix2(angle * float(M_PI) / 180.0f, offset.p);
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
	body->SetCenterPosition(transform.p, transform.Angle());
	SetTransform(transform);

	// call parent init
	Entity::Init();

	// constrain to the offset position
	b2PrismaticJointDef joint;
	joint.body1 = player->GetBody();
	joint.body2 = GetBody();
	joint.anchorPoint = joint.body1->GetCenterPosition();
	joint.enableLimit = true;
	joint.motorForce = 100.0f;
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

	// cancel velocity offset
	// (prevents "wiggle" in the joint constraint)
	b2Body *player_body = player->GetBody();
	b2Vec2 dv((player_body->GetLinearVelocity() - body->GetLinearVelocity() - b2Cross(player_body->GetAngularVelocity(), player_body->GetCenterPosition() - body->GetCenterPosition())));
	body->ApplyImpulse(body->GetMass() * dv, body->GetCenterPosition());

	// advance fire timer
	mDelay -= aStep * mCycle;

	// if ready to fire...
	if (mDelay <= 0.0f)
	{
		// if triggered...
		if ((*input)[Input::FIRE_PRIMARY])
		{
			// if firing on this phase...
			if (mPhase == 0)
			{
				// for each shot...
				for (int i = 0; i < 2; i++)
				{
					// generate a bullet
					const Vector2 d = GUNNER_BULLET_DIR[i];
					Bullet *bullet = Bullet::pool.construct(0, 0xd85669f0 /* "playerbullet" */);
					bullet->SetTransform(angle_1, posit_1);
					bullet->SetVelocity(bullet->GetTransform().Rotate(d) * GUNNER_BULLET_SPEED);
					bullet->Init();
					bullet->AddToWorld();
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
			mDelay += 0.25f;
		}
		else
		{
			// clamp fire delay
			mDelay = 0.0f;
		}
	}
}

// Gunner Simulate
void Gunner::Simulate(float aStep)
{
	if (!player)
		return;

}

// Gunner Render
void Gunner::Render(const Matrix2 &transform)
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
