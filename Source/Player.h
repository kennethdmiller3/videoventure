#pragma once

#include "Controller.h"
#include "Updatable.h"

// player

// player template
class PlayerTemplate
{
public:
	unsigned int mSpawn;
	int mLives;
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
	void Spawn(void);

	// attach to an object
	void Attach(unsigned int aAttach);

	// detach from an object
	void Detach(void);

	// died
	void OnDeath(unsigned int aId, unsigned int aSourceId);

	// got a kill
	void GotKill(unsigned int aId, unsigned int aKillId);
};

// player controller
class PlayerController : 
	public Controller
{
public:
	// constructor
	PlayerController(unsigned int aId = 0);

	// destructor
	~PlayerController(void);

	// control
	virtual void Control(float aStep);
};

namespace Database
{
	extern Typed<PlayerTemplate> playertemplate;
	extern Typed<Player *> player;
	extern Typed<PlayerController *> playercontroller;
}