#include "StdAfx.h"
#include "PlayerController.h"
#include "Player.h"
#include "Entity.h"
#include "Ship.h"


namespace Database
{
	Typed<bool> playercontrollertemplate(0xec81fd12 /* "playercontrollertemplate" */);
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
				bool &playercontroller = Database::playercontrollertemplate.Open(aId);
				int value = playercontroller;
				if (element->QueryIntAttribute("alignmove", &value) == TIXML_SUCCESS)
					playercontroller = value != 0;
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

// player controller ontrol
void PlayerController::Control(float aStep)
{
	// get parent entity
	Entity *entity = Database::entity.Get(mId);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// TO DO: support multiple players

	// set move input
	mMove.x = input[Input::MOVE_HORIZONTAL];
	mMove.y = input[Input::MOVE_VERTICAL];
	mMove = transform.Unrotate(mMove);

	// set turn input
	extern Vector2 camerapos[];
	extern float VIEW_SIZE;
	Vector2 mAim;
	mAim.x = camerapos[1].x + input[Input::AIM_HORIZONTAL] * 120 * VIEW_SIZE / 320;
	mAim.y = camerapos[1].y + input[Input::AIM_VERTICAL] * 120 * VIEW_SIZE / 320;
	mAim = transform.Untransform(mAim);

	// turn towards target direction
	const ShipTemplate &ship = Database::shiptemplate.Get(mId);	// <-- hack!
	if (ship.mMaxOmega != 0.0f)
	{
		bool alignfacing = Database::playercontrollertemplate.Get(mId);
		const Vector2 face(alignfacing ? mMove : mAim * 0.0625);
		float aim_angle = -std::min(face.LengthSq(), 1.0f) * atan2f(face.x, face.y);
		mTurn = Clamp(aim_angle / (ship.mMaxOmega * aStep), -1.0f, 1.0f);
	}

	// set fire input
	mFire[0] = input[Input::FIRE_PRIMARY] != 0.0f;
	mFire[1] = input[Input::FIRE_SECONDARY] != 0.0f;
	mFire[2] = input[Input::FIRE_CHANNEL3] != 0.0f;
	mFire[3] = input[Input::FIRE_CHANNEL4] != 0.0f;
}
