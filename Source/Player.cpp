#include "StdAfx.h"
#include "Player.h"
#include "Entity.h"
#include "Bullet.h"
#include "Damagable.h"
#ifdef USE_CAPTURABLE
#include "Capturable.h"
#endif
#include "Team.h"
#include "Points.h"
#include "PointsOverlay.h"
#include "Sound.h"

#include "Ship.h"
#include "Resource.h"

namespace Database
{
	Typed<PlayerTemplate> playertemplate(0x4893610a /* "playertemplate" */);
	Typed<Player *> player(0x2c99c300 /* "player" */);

	namespace Loader
	{
		static void PlayerConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			PlayerTemplate &player = Database::playertemplate.Open(aId);
			player.Configure(element, aId);
			Database::playertemplate.Close(aId);
		}
		Configure playerconfigure(0x2c99c300 /* "player" */, PlayerConfigure);
	}

	namespace Initializer
	{
		static void PlayerActivate(unsigned int aId)
		{
			const PlayerTemplate &playertemplate = Database::playertemplate.Get(aId);
			Player *player = new Player(playertemplate, aId);
			Database::player.Put(aId, player);
			player->Activate();
		}
		Activate playeractivate(0x4893610a /* "playertemplate" */, PlayerActivate);

		static void PlayerDeactivate(unsigned int aId)
		{
			if (Player *player = Database::player.Get(aId))
			{
				delete player;
				Database::player.Delete(aId);
			}
		}
		Deactivate playerdeactivate(0x4893610a /* "playertemplate" */, PlayerDeactivate);
	}
}


Signal<void(unsigned int)> Player::sJoin;
Signal<void(unsigned int)> Player::sQuit;


/*
// PLAYER TEMPLATE
*/

// player template constructor
PlayerTemplate::PlayerTemplate(void)
: mSpawn(0)
, mStart(0.0f)
, mCycle(0.0f)
, mLives(INT_MAX)
, mFirst(INT_MAX)
, mExtra(INT_MAX)
{
}

// player template configure
bool PlayerTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	if (const char *spawn = element->Attribute("name"))
		mSpawn = Hash(spawn);
	element->QueryFloatAttribute("start", &mStart);
	element->QueryFloatAttribute("cycle", &mCycle);
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
, mTimer(0.0f)
, mAttach(0)
, mLives(0)
, mScore(0)
{
	SetAction(Action(this, &Player::Update));
}

// player constructor
Player::Player(const PlayerTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mTimer(aTemplate.mStart)
, mAttach(0)
, mLives(aTemplate.mLives)
, mScore(0)
{
	SetAction(Action(this, &Player::Update));

	// notify join listeners
	sJoin(mId);

	{
		// add a kill listener
		Damagable::KillSignal &signal = Database::killsignal.Open(mId);
		signal.Connect(this, &Player::GotKill);
		Database::killsignal.Close(mId);
	}

#ifdef USE_CAPTURABLE
	{
		// add a capture listener
		Capturable::CaptureSignal &signal = Database::capturelistener.Open(mId);
		signal.Connect(this, &Player::GotKill);
		Database::capturelistener.Close(mId);
	}
#endif
	
	// add points overlay
	Database::pointsoverlay.Put(aId, new PointsOverlay(aId));
}

// player destructor
Player::~Player(void)
{
	// remove points overlay
	delete Database::pointsoverlay.Get(mId);
	Database::pointsoverlay.Delete(mId);

#if USE_CAPTURABLE
	{
		// remove any capture listener
		Capturable::CaptureSignal &signal = Database::capturelistener.Open(mId);
		signal.Disconnect(this, &Player::GotKill);
		Database::capturelistener.Close(mId);
	}
#endif

	{
		// remove any kill listener
		Damagable::KillSignal &signal = Database::killsignal.Open(mId);
		signal.Disconnect(this, &Player::GotKill);
		Database::killsignal.Close(mId);
	}

	// notify leave listeners
	sQuit(mId);
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

	// count down timer
	mTimer -= aStep;

	// if the timer elapses, and there are lives left...
	if (mTimer <= 0.0f && mLives > 0)
	{
		// spawn a new entity
		Attach(Spawn());
	}
}

// player attach
void Player::Attach(unsigned int aAttach)
{
	// do nothing if attached
	if (mAttach)
		return;

#ifdef DEBUG_PLAYER_ATTACH
	DebugPrint("%s attach %s\n", Database::name.Get(mId).c_str(), Database::name.Get(aAttach).c_str());
#endif

	// attach to entity
	mAttach = aAttach;

	// add a death listener
	Damagable::DeathSignal &signal = Database::deathsignal.Open(mAttach);
	signal.Connect(this, &Player::OnDeath);
	Database::deathsignal.Close(mAttach);
}

// player detach
void Player::Detach(unsigned int aAttach)
{
	// do nothing if not attached
	if (!mAttach || mAttach != aAttach)
		return;

#ifdef DEBUG_PLAYER_ATTACH
	DebugPrint("%s detach %s\n", Database::name.Get(mId).c_str(), Database::name.Get(aAttach).c_str());
#endif

	// remove any death listener
	Damagable::DeathSignal &signal = Database::deathsignal.Open(mAttach);
	signal.Disconnect(this, &Player::OnDeath);
	Database::deathsignal.Close(mAttach);

	// detach from entity
	mAttach = 0;

	// start delay
	mTimer = Database::playertemplate.Get(mId).mCycle;
}

// player spawn
unsigned int Player::Spawn(void)
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
		unsigned int spawnId = Database::Instantiate(playertemplate.mSpawn, mId, mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());

		// done for now
		Deactivate();

		// return the spawned entity
		return spawnId;
	}

	return 0;
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
			PlaySoundCue(mAttach, 0x62b13a2b /* "extralife" */);
		}
	}

	// add value to score
	mScore += aValue;
}
