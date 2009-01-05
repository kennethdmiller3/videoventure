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
	Typed<fastdelegate::FastDelegate<void (unsigned int)> > playerjoin(0xd15784b0 /* "playerjoin" */);
	Typed<fastdelegate::FastDelegate<void (unsigned int)> > playerquit(0x33bcfaff /* "playerquit" */);
	Typed<PlayerTemplate> playertemplate(0x4893610a /* "playertemplate" */);
	Typed<Player *> player(0x2c99c300 /* "player" */);

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
	}
}

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
bool PlayerTemplate::Configure(const TiXmlElement *element, unsigned int aId)
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
	for (Database::Typed<fastdelegate::FastDelegate<void (unsigned int)> >::Iterator itor(&Database::playerjoin); itor.IsValid(); ++itor)
		itor.GetValue()(mId);

	{
		// add a kill listener
		Database::Typed<Damagable::KillListener> &listeners = Database::killlistener.Open(mId);
		listeners.Put(Database::Key(this), Damagable::KillListener(this, &Player::GotKill));
		Database::killlistener.Close(mId);
	}

#ifdef USE_CAPTURABLE
	{
		// add a capture listener
		Database::Typed<Capturable::CaptureListener> &listeners = Database::capturelistener.Open(mId);
		listeners.Put(Database::Key(this), Capturable::KillListener(this, &Player::GotKill));
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
		Database::Typed<Capturable::CaptureListener> &listeners = Database::capturelistener.Open(mId);
		listeners.Delete(Database::Key(this));
		Database::capturelistener.Close(mId);
	}
#endif

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
	Database::Typed<Damagable::DeathListener> &listeners = Database::deathlistener.Open(mAttach);
	listeners.Put(Database::Key(this), Damagable::DeathListener(this, &Player::OnDeath));
	Database::deathlistener.Close(mAttach);
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
	Database::Typed<Damagable::DeathListener> &listeners = Database::deathlistener.Open(mAttach);
	listeners.Delete(Database::Key(this));
	Database::deathlistener.Close(mAttach);

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
