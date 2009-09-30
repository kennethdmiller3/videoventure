#include "StdAfx.h"
#include "Player.h"
#include "PlayerHUD.h"
#include "PlayerOverlayAmmo.h"
#include "PlayerOverlayGameOver.h"
#include "PlayerOverlayHealth.h"
#include "PlayerOverlayLevel.h"
#include "PlayerOverlayLives.h"
#include "PlayerOverlayReticule.h"
#include "PlayerOverlayScore.h"
#include "PlayerOverlaySpecial.h"


namespace Database
{
	Typed<PlayerHUD *> playerhud(0x8e522e29 /* "this" */, 2);
}


//
// PLAYER OVERLAY: ROOT
//

// constructor
PlayerHUD::PlayerHUD(unsigned int aPlayerId = 0)
: Overlay(aPlayerId)
{
	// TO DO: find a way to create player overlays from the level file

	// attach player score overlay
	PlayerOverlayScore *playeroverlayscore = new PlayerOverlayScore(aPlayerId);
	playeroverlayscore->AttachLast(this);

	// attach player health overlay
	PlayerOverlayHealth *playeroverlayhealth = new PlayerOverlayHealth(aPlayerId);
	playeroverlayhealth->AttachLast(this);

	// attach player lives overlay
	PlayerOverlayLives *playeroverlaylives = new PlayerOverlayLives(aPlayerId);
	playeroverlaylives->AttachLast(this);

	// attach player ammo overlay
	PlayerOverlayAmmo *playeroverlayammo = new PlayerOverlayAmmo(aPlayerId);
	playeroverlayammo->AttachLast(this);

	// attach player level overlay
	PlayerOverlayLevel *playeroverlaylevel = new PlayerOverlayLevel(aPlayerId);
	playeroverlaylevel->AttachLast(this);

	// attach player special overlay
	PlayerOverlaySpecial *playeroverlayspecial = new PlayerOverlaySpecial(aPlayerId);
	playeroverlayspecial->AttachLast(this);

	// attach player reticule overlay
	PlayerOverlayReticule *playeroverlayreticule = new PlayerOverlayReticule(aPlayerId);
	playeroverlayreticule->AttachLast(this);
	playeroverlayreticule->Activate();

	// attach player gameover overlay
	PlayerOverlayGameOver *playeroverlaygameover = new PlayerOverlayGameOver(aPlayerId);
	playeroverlaygameover->AttachLast(this);
}

// destructor
PlayerHUD::~PlayerHUD()
{
}
