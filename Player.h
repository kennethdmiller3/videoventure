#pragma once

#include "Updatable.h"

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
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

// player
class Player : public Updatable
{
public:
	float mTimer;
	unsigned int mAttach;
	int mLives;
	int mScore;

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
	extern Typed<fastdelegate::FastDelegate<void (unsigned int)> > playerjoin;
	extern Typed<fastdelegate::FastDelegate<void (unsigned int)> > playerquit;
	extern Typed<PlayerTemplate> playertemplate;
	extern Typed<Player *> player;
}