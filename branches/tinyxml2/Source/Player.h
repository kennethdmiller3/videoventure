#pragma once

#include "Updatable.h"
#include "Signal.h"


// player

// player template
class PlayerTemplate
{
public:
	unsigned int mSpawn;
	float mStart;
	float mCycle;
	int mLives;
	int mFirst;
	int mExtra;

public:
	// constructor
	PlayerTemplate(void);

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

// player
class GAME_API Player : public Updatable
{
public:
	float mTimer;
	unsigned int mAttach;
	int mLives;
	int mScore;

	static Signal<void(unsigned int)> sJoin;
	static Signal<void(unsigned int)> sQuit;

public:
	// default constructor
	Player(void);

	// constructor
	Player(const PlayerTemplate &aTemplate, unsigned int aId);

	// destructor
	~Player(void);

	// update
	void Update(float aStep);

	// spawn
	unsigned int Spawn(void);

	// attach to an object
	void Attach(unsigned int aAttach);

	// detach from an object
	void Detach(unsigned int aAttach);

	// died
	void OnDeath(unsigned int aId, unsigned int aSourceId);

	// got a kill
	void GotKill(unsigned int aId, unsigned int aKillId);
};

namespace Database
{
	extern GAME_API Typed<PlayerTemplate> playertemplate;
	extern GAME_API Typed<Player *> player;
}