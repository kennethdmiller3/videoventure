#include "StdAfx.h"
#include "Player.h"
#include "Entity.h"
#include "Bullet.h"
#include "Damagable.h"
#include "Capturable.h"
#include "Team.h"
#include "Points.h"
#include "Sound.h"

#include "Ship.h"
#include "Resource.h"

#include "Overlay.h"

extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);

// points overlay
class PointsOverlay : public Overlay
{
	struct PointsItem
	{
		Vector2 mPosition;
		int mValue : 24;
		int mCombo : 8;
		float mTime;
	};
	PointsItem mItems[256];
	int mItemFirst;
	int mItemLast;

public:
	PointsOverlay(void);
	PointsOverlay(unsigned int aId);

	void AddItem(const Vector2 &aPosition, int aValue, int aCombo);

	void Render(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle);
};

namespace Database
{
	Typed<fastdelegate::FastDelegate<void (unsigned int)> > playerjoin(0xd15784b0 /* "playerjoin" */);
	Typed<fastdelegate::FastDelegate<void (unsigned int)> > playerquit(0x33bcfaff /* "playerquit" */);
	Typed<PlayerTemplate> playertemplate(0x4893610a /* "playertemplate" */);
	Typed<Player *> player(0x2c99c300 /* "player" */);
	Typed<bool> playercontrollertemplate(0xec81fd12 /* "playercontrollertemplate" */);
	Typed<PlayerController *> playercontroller(0x7a57caa8 /* "playercontroller" */);
	Typed<PointsOverlay *> pointsoverlay(0x325ede2e /* "pointsoverlay" */);

	namespace Loader
	{
		class PlayerLoader
		{
		public:
			PlayerLoader()
			{
				AddConfigure(0x2c99c300 /* "player" */, Entry(this, &PlayerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				PlayerTemplate &player = Database::playertemplate.Open(aId);
				player.Configure(element, aId);
				Database::playertemplate.Close(aId);
			}
		}
		playerloader;

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
		class PlayerInitializer
		{
		public:
			PlayerInitializer()
			{
				AddActivate(0x4893610a /* "playertemplate" */, Entry(this, &PlayerInitializer::Activate));
				AddDeactivate(0x4893610a /* "playertemplate" */, Entry(this, &PlayerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const PlayerTemplate &playertemplate = Database::playertemplate.Get(aId);
				Player *player = new Player(playertemplate, aId);
				Database::player.Put(aId, player);
				player->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Player *player = Database::player.Get(aId))
				{
					delete player;
					Database::player.Delete(aId);
				}
			}
		}
		playerinitializer;

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

/*
// PLAYER TEMPLATE
*/

// player template constructor
PlayerTemplate::PlayerTemplate(void)
: mSpawn(0)
, mLives(INT_MAX)
, mFirst(INT_MAX)
, mExtra(INT_MAX)
{
}

// player template configure
bool PlayerTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	if (const char *spawn = element->Attribute("name"))
		mSpawn = Hash(spawn);
	element->QueryIntAttribute("lives", &mLives);
	element->QueryIntAttribute("firstextra", &mFirst);
	element->QueryIntAttribute("extra", &mExtra);
	return true;
}


/*
// PLAYER
*/

// player default constructor
Player::Player(void)
: Updatable(0)
, mAttach(0)
, mLives(0)
, mScore(0)
{
	SetAction(Action(this, &Player::Update));
}

// player constructor
Player::Player(const PlayerTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mAttach(0)
, mLives(aTemplate.mLives)
, mScore(0)
{
	SetAction(Action(this, &Player::Update));

	// notify join listeners
	for (Database::Typed<fastdelegate::FastDelegate<void (unsigned int)> >::Iterator itor(&Database::playerjoin); itor.IsValid(); ++itor)
		itor.GetValue()(mId);

	{
		// add a kill listener
		Database::Typed<Damagable::KillListener> &listeners = Database::killlistener.Open(mId);
		Damagable::KillListener &listener = listeners.Open(Database::Key(this));
		listener.bind(this, &Player::GotKill);
		listeners.Close(Database::Key(this));
		Database::killlistener.Close(mId);
	}

	{
		// add a capture listener
		Database::Typed<Capturable::CaptureListener> &listeners = Database::capturelistener.Open(mId);
		Capturable::CaptureListener &listener = listeners.Open(Database::Key(this));
		listener.bind(this, &Player::GotKill);
		listeners.Close(Database::Key(this));
		Database::capturelistener.Close(mId);
	}
	
	// add points overlay
	Database::pointsoverlay.Put(aId, new PointsOverlay(aId));
}

// player destructor
Player::~Player(void)
{
	// remove points overlay
	delete Database::pointsoverlay.Get(mId);
	Database::pointsoverlay.Delete(mId);

	{
		// remove any capture listener
		Database::Typed<Capturable::CaptureListener> &listeners = Database::capturelistener.Open(mId);
		listeners.Delete(Database::Key(this));
		Database::capturelistener.Close(mId);
	}

	{
		// remove any kill listener
		Database::Typed<Damagable::KillListener> &listeners = Database::killlistener.Open(mId);
		listeners.Delete(Database::Key(this));
		Database::killlistener.Close(mId);
	}

	// notify leave listeners
	for (Database::Typed<fastdelegate::FastDelegate<void (unsigned int)> >::Iterator itor(&Database::playerquit); itor.IsValid(); ++itor)
		itor.GetValue()(mId);
}

// player update
void Player::Update(float aStep)
{
	// if attached to an entity...
	if (mAttach)
	{
		// if the entity is still alive...
		if (Database::entity.Get(mAttach))
		{
			// wait
			return;
		}
		else
		{
			// detach
			Detach(mAttach);
		}
	}

	// if there are lives left...
	if (mLives > 0)
	{
		// spawn a new entity
		Spawn();
	}
}

// player attach
void Player::Attach(unsigned int aAttach)
{
	// do nothing if attached
	if (mAttach)
		return;

	// attach to entity
	mAttach = aAttach;

	// add a death listener
	Database::Typed<Damagable::DeathListener> &listeners = Database::deathlistener.Open(mAttach);
	Damagable::DeathListener &listener = listeners.Open(Database::Key(this));
	listener.bind(this, &Player::OnDeath);
	listeners.Close(Database::Key(this));
	Database::deathlistener.Close(mAttach);
}

// player detach
void Player::Detach(unsigned int aAttach)
{
	// do nothing if not attached
	if (!mAttach || mAttach != aAttach)
		return;

	// remove any death listener
	Database::Typed<Damagable::DeathListener> &listeners = Database::deathlistener.Open(mAttach);
	listeners.Delete(Database::Key(this));
	Database::deathlistener.Close(mAttach);

	// detach from enemy
	mAttach = 0;
}

// player spawn
void Player::Spawn(void)
{
	// get the spawner entity
	Entity *entity = Database::entity.Get(mId);
	if (entity)
	{
		// use a life
		if (mLives < INT_MAX)
			--mLives;

		// get the player template
		const PlayerTemplate &playertemplate = Database::playertemplate.Get(mId);

		// instantiate the spawn entity
		// TO DO: use a named spawn point
		Database::Instantiate(playertemplate.mSpawn, mId, mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());

		// done for now
		Deactivate();
	}
}

// player death notification
void Player::OnDeath(unsigned int aId, unsigned int aSourceId)
{
	Activate();
}

// player kill notification
void Player::GotKill(unsigned int aId, unsigned int aKillId)
{
	// get point value
	int aValue = Database::points.Get(aKillId);
	if (aValue == 0)
		return;

	// no points for team-kill
	if (Database::team.Get(aId) == Database::team.Get(aKillId))
		return;

	// get combo value
	int aCombo = 1;
	if (const int *combo = Database::hitcombo.Find(aKillId))
		aCombo = *combo;

	// show a point value indicator (HACK)
	Database::pointsoverlay.Get(aId)->AddItem(Database::entity.Get(aKillId)->GetPosition(), aValue, aCombo);

	// apply combo multiplier
	aValue *= aCombo;

	// get the player template
	const PlayerTemplate &playertemplate = Database::playertemplate.Get(mId);

	// if extra lives enabled...
	if (playertemplate.mExtra > 0)
	{
		// extra lives
		int extra = ((mScore + aValue) >= playertemplate.mFirst) - (mScore >= playertemplate.mFirst) + ((mScore + aValue) / playertemplate.mExtra) - (mScore / playertemplate.mExtra);
		if (extra > 0)
		{
			// add any extra lives
			mLives += extra;

			// add any extra bombs (HACK)
			if (Resource *resource = Database::resource.Get(mId).Get(0xd940d530 /* "special" */))
				resource->Add(mId, float(extra));

			// trigger sound cue
			PlaySound(mAttach, 0x62b13a2b /* "extralife" */);
		}
	}

	// add value to score
	mScore += aValue;
}


/*
// PLAYER CONTROLLER
*/

// player controller constructor
PlayerController::PlayerController(unsigned int aId)
: Controller(aId)
{
	SetAction(Action(this, &PlayerController::Control));

	unsigned int aOwnerId = Database::owner.Get(mId);
	if (Player *player = Database::player.Get(aOwnerId))
	{
		player->Attach(mId);
	}
}

// player controller destructor
PlayerController::~PlayerController(void)
{
	unsigned int aOwnerId = Database::owner.Get(mId);
	if (Player *player = Database::player.Get(aOwnerId))
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
	const Matrix2 transform(entity->GetTransform());

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


// points overlay
PointsOverlay::PointsOverlay(void)
	: Overlay(0), mItemFirst(0), mItemLast(0)
{
}

PointsOverlay::PointsOverlay(unsigned int aId)
	: Overlay(aId), mItemFirst(0), mItemLast(0)
{
	SetAction(Action(this, &PointsOverlay::Render));
}

void PointsOverlay::AddItem(const Vector2 &aPosition, int aValue, int aCombo)
{
	// show if adding an item
	if (mItemFirst == mItemLast)
		Show();

	mItems[mItemLast].mPosition = aPosition;
	mItems[mItemLast].mValue = aValue;
	mItems[mItemLast].mCombo = aCombo;
	mItems[mItemLast].mTime = 2.0f;

	mItemLast = (mItemLast + 1) % SDL_arraysize(mItems);

	if (mItemFirst == mItemLast)
		mItemFirst = (mItemLast + 1) % SDL_arraysize(mItems);
}

void PointsOverlay::Render(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	extern float VIEW_SIZE;
	extern int SCREEN_HEIGHT;
	extern int SCREEN_WIDTH;
	extern Vector2 camerapos[2];
	extern float frame_time;

	// set projection
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glFrustum( -0.5*VIEW_SIZE, 0.5*VIEW_SIZE, 0.5f*VIEW_SIZE*SCREEN_HEIGHT/SCREEN_WIDTH, -0.5f*VIEW_SIZE*SCREEN_HEIGHT/SCREEN_WIDTH, 256.0f*1.0f, 256.0f*5.0f );

	// set base modelview matrix
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	glTranslatef( 0.0f, 0.0f, -256.0f );
	glScalef( -1.0f, -1.0f, -1.0f );

	// push camera transform
	glPushMatrix();

	// get interpolated track position
	Vector2 viewpos(Lerp(camerapos[0], camerapos[1], sim_fraction));

	// set view position
	glTranslatef( -viewpos.x, -viewpos.y, 0 );

	// start drawing
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);
	glBegin(GL_QUADS);

	// for each points item
	for (int i = mItemFirst; i != mItemLast; i = (i + 1) % SDL_arraysize(mItems))
	{
		// get the item
		PointsItem &item = mItems[i];

		// get string
		char buf[16];
		if (item.mCombo > 1)
			sprintf(buf, "%dx%d", item.mValue, item.mCombo);
		else
			sprintf(buf, "%d", item.mValue);

		// draw point value
		glColor4f(1.0f, 1.0f, 1.0f, std::min(item.mTime, 1.0f));
		float w = 4 * VIEW_SIZE / 320;
		OGLCONSOLE_DrawString(buf, item.mPosition.x + w * 0.5f * strlen(buf), item.mPosition.y - w * 0.5f, -w, w, 0);

		// count down time
		item.mTime -= frame_time;

		// delete if expired
		if (item.mTime <= 0.0f)
			mItemFirst = (i + 1) % SDL_arraysize(mItems);
	}

	// finish drawing
	glEnd();
	glDisable(GL_TEXTURE_2D);

	// reset camera transform
	glPopMatrix();

	// reset camera transform
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// hide if empty...
	if (mItemFirst == mItemLast)
		Hide();
}
