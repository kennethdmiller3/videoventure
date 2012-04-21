#include "StdAfx.h"
#include "PlayerController.h"
#include "Player.h"
#include "Entity.h"
#include "Ship.h"


namespace Database
{
	Typed<PlayerControllerTemplate> playercontrollertemplate(0xec81fd12 /* "playercontrollertemplate" */);
	Typed<PlayerController *> playercontroller(0x7a57caa8 /* "playercontroller" */);

	namespace Loader
	{
		class PlayerControllerLoader
		{
		public:
			PlayerControllerLoader()
			{
				AddConfigure(0x7a57caa8 /* "playercontroller" */, Entry(this, &PlayerControllerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				PlayerControllerTemplate &playercontroller = Database::playercontrollertemplate.Open(aId);
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					switch (Hash(child->Value()))
					{
					case 0x383251f6 /* "aim" */:
						switch(Hash(child->Attribute("type")))
						{
						case 0xada7afdb /* "none" */: playercontroller.mAim = PlayerControllerTemplate::NONE; break;
						case 0xa73b0f65 /* "movesteer" */: playercontroller.mAim = PlayerControllerTemplate::MOVESTEER; break;
						case 0xa4219fff /* "movelocal" */: playercontroller.mAim = PlayerControllerTemplate::MOVELOCAL; break;
						case 0x55b3fdb0 /* "moveworld" */: playercontroller.mAim = PlayerControllerTemplate::MOVEWORLD; break;
						case 0x49cd54a6 /* "movecursor" */: playercontroller.mAim = PlayerControllerTemplate::MOVECURSOR; break;
						case 0x3b8907cb /* "aimsteer" */: playercontroller.mAim = PlayerControllerTemplate::AIMSTEER; break;
						case 0x631798dd /* "aimlocal" */: playercontroller.mAim = PlayerControllerTemplate::AIMLOCAL; break;
						case 0xaee0ceba /* "aimworld" */: playercontroller.mAim = PlayerControllerTemplate::AIMWORLD; break;
						case 0xe9ec1504 /* "aimcursor" */: playercontroller.mAim = PlayerControllerTemplate::AIMCURSOR; break;
						case 0x124aec70 /* "left" */: playercontroller.mAim = PlayerControllerTemplate::LEFT; break;
						case 0x78e32de5 /* "right" */: playercontroller.mAim = PlayerControllerTemplate::RIGHT; break;
						case 0x43430b20 /* "up" */: playercontroller.mAim = PlayerControllerTemplate::UP; break;
						case 0x3db9b915 /* "down" */: playercontroller.mAim = PlayerControllerTemplate::DOWN; break;
						}
						break;

					case 0x184b0014 /* "move" */:
						switch(Hash(child->Attribute("type")))
						{
						case 0xada7afdb /* "none" */: playercontroller.mMove = PlayerControllerTemplate::NONE; break;
						case 0xa4219fff /* "movelocal" */: playercontroller.mMove = PlayerControllerTemplate::MOVELOCAL; break;
						case 0x55b3fdb0 /* "moveworld" */: playercontroller.mMove = PlayerControllerTemplate::MOVEWORLD; break;
						case 0x631798dd /* "aimlocal" */: playercontroller.mMove = PlayerControllerTemplate::AIMLOCAL; break;
						case 0xaee0ceba /* "aimworld" */: playercontroller.mMove = PlayerControllerTemplate::AIMWORLD; break;
						case 0x124aec70 /* "left" */: playercontroller.mMove = PlayerControllerTemplate::LEFT; break;
						case 0x78e32de5 /* "right" */: playercontroller.mMove = PlayerControllerTemplate::RIGHT; break;
						case 0x43430b20 /* "up" */: playercontroller.mMove = PlayerControllerTemplate::UP; break;
						case 0x3db9b915 /* "down" */: playercontroller.mMove = PlayerControllerTemplate::DOWN; break;
						}
						break;

					case 0x82971c71 /* "scale" */:
						child->QueryFloatAttribute("strafe", &playercontroller.mScale.p.x);
						child->QueryFloatAttribute("thrust", &playercontroller.mScale.p.y);
						child->QueryFloatAttribute("turn", &playercontroller.mScale.a);
						break;

					case 0x3b391274 /* "add" */:
						child->QueryFloatAttribute("strafe", &playercontroller.mAdd.p.x);
						child->QueryFloatAttribute("thrust", &playercontroller.mAdd.p.y);
						child->QueryFloatAttribute("turn", &playercontroller.mAdd.a);
						break;
					}
				}

				Database::playercontrollertemplate.Close(aId);
			}
		}
		playercontrollerloader;
	}

	namespace Initializer
	{
		class PlayerControllerInitializer
		{
		public:
			PlayerControllerInitializer()
			{
				AddActivate(0xec81fd12 /* "playercontrollertemplate" */, Entry(this, &PlayerControllerInitializer::Activate));
				AddDeactivate(0xec81fd12 /* "playercontrollertemplate" */, Entry(this, &PlayerControllerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				PlayerController *playercontroller = new PlayerController(aId);
				Database::playercontroller.Put(aId, playercontroller);
				Database::controller.Put(aId, playercontroller);

				playercontroller->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (PlayerController *playercontroller = Database::playercontroller.Get(aId))
				{
					delete playercontroller;
					Database::playercontroller.Delete(aId);
					Database::controller.Delete(aId);
				}
			}
		}
		playercontrollerinitializer;
	}
}


// player controller constructor
PlayerController::PlayerController(unsigned int aId)
: Controller(aId)
{
	SetAction(Action(this, &PlayerController::Control));

	unsigned int aCreatorId = Database::creator.Get(mId);
	if (Player *player = Database::player.Get(aCreatorId))
	{
		player->Attach(mId);
	}
}

// player controller destructor
PlayerController::~PlayerController(void)
{
	unsigned int aCreatorId = Database::creator.Get(mId);
	if (Player *player = Database::player.Get(aCreatorId))
	{
		player->Detach(mId);
	}
}

// turn to the specified vector
static float TurnLocal(const Vector2 &aDir)
{
	// turn towards direction
	return -std::min(aDir.LengthSq(), 1.0f) * atan2f(aDir.x, aDir.y);
}

// player controller ontrol
void PlayerController::Control(float aStep)
{
	// get parent entity
	Entity *entity = Database::entity.Get(mId);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get player controller template
	const PlayerControllerTemplate &controllertemplate = Database::playercontrollertemplate.Get(mId);


	// TO DO: support multiple players
	// TO DO: replace switch statements with behaviors

	// set move input
	switch (controllertemplate.mMove)
	{
	case PlayerControllerTemplate::NONE:
		mMove.x = mMove.y = 0;
		break;

	case PlayerControllerTemplate::MOVELOCAL:
		mMove.x = input[Input::MOVE_HORIZONTAL];
		mMove.y = input[Input::MOVE_VERTICAL];
		break;

	case PlayerControllerTemplate::MOVEWORLD:
		mMove.x = input[Input::MOVE_HORIZONTAL];
		mMove.y = input[Input::MOVE_VERTICAL];
		mMove = transform.Unrotate(mMove);
		break;

	case PlayerControllerTemplate::AIMLOCAL:
		mMove.x = input[Input::AIM_HORIZONTAL];
		mMove.y = input[Input::AIM_VERTICAL];
		break;

	case PlayerControllerTemplate::AIMWORLD:
		mMove.x = input[Input::AIM_HORIZONTAL];
		mMove.y = input[Input::AIM_VERTICAL];
		mMove = transform.Unrotate(mMove);
		break;

	case PlayerControllerTemplate::LEFT:
		mMove = transform.Unrotate(Vector2(1, 0));
		break;

	case PlayerControllerTemplate::RIGHT:
		mMove = transform.Unrotate(Vector2(-1, 0));
		break;

	case PlayerControllerTemplate::UP:
		mMove = transform.Unrotate(Vector2(0, 1));
		break;

	case PlayerControllerTemplate::DOWN:
		mMove = transform.Unrotate(Vector2(0, -1));
		break;
	};

	// set turn input
	switch(controllertemplate.mAim)
	{
	case PlayerControllerTemplate::NONE:
		mAim.x = mAim.y = 0;
		mTurn = 0;
		break;

	case PlayerControllerTemplate::MOVESTEER:
		mAim.x = mAim.y = 0;
		mTurn = -input[Input::MOVE_HORIZONTAL];
		break;

	case PlayerControllerTemplate::MOVELOCAL:
		mAim.x = input[Input::MOVE_HORIZONTAL];
		mAim.y = input[Input::MOVE_VERTICAL];
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::MOVEWORLD:
		mAim.x = input[Input::MOVE_HORIZONTAL];
		mAim.y = input[Input::MOVE_VERTICAL];
		mAim = transform.Unrotate(mAim);
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::MOVECURSOR:
		mAim = transform.Untransform(camerapos[1] + Vector2(input[Input::MOVE_HORIZONTAL], input[Input::MOVE_VERTICAL]) * 120 * VIEW_SIZE / 240);
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::AIMSTEER:
		mAim.x = mAim.y = 0;
		mTurn = -input[Input::AIM_HORIZONTAL];
		break;

	case PlayerControllerTemplate::AIMLOCAL:
		mAim.x = input[Input::AIM_HORIZONTAL];
		mAim.y = input[Input::AIM_VERTICAL];
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::AIMWORLD:
		mAim.x = input[Input::AIM_HORIZONTAL];
		mAim.y = input[Input::AIM_VERTICAL];
		mAim = transform.Unrotate(mAim);
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::AIMCURSOR:
		mAim = transform.Untransform(camerapos[1] + Vector2(input[Input::AIM_HORIZONTAL], input[Input::AIM_VERTICAL]) * 120 * VIEW_SIZE / 240);
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::LEFT:
		mAim = transform.Unrotate(Vector2(1, 0));
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::RIGHT:
		mAim = transform.Unrotate(Vector2(-1, 0));
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::UP:
		mAim = transform.Unrotate(Vector2(0, 1));
		mTurn = TurnLocal(mAim) / aStep;
		break;

	case PlayerControllerTemplate::DOWN:
		mAim = transform.Unrotate(Vector2(0, -1));
		mTurn = TurnLocal(mAim) / aStep;
		break;
	}

	// apply scale and add
	mMove = mMove * controllertemplate.mScale.p + controllertemplate.mAdd.p;
	mTurn = mTurn * controllertemplate.mScale.a + controllertemplate.mAdd.a;

	// clamp to limits
	float moveSq = mMove.LengthSq();
	if (moveSq > 1.0f)
		mMove *= InvSqrt(moveSq);
	mTurn = Clamp(mTurn, -1.0f, 1.0f);

	// set fire input
	mFire[0] = input[Input::FIRE_PRIMARY];
	mFire[1] = input[Input::FIRE_SECONDARY];
	mFire[2] = input[Input::FIRE_CHANNEL3];
	mFire[3] = input[Input::FIRE_CHANNEL4];
}
