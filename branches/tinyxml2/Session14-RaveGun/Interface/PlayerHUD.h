#pragma once

#include "Updatable.h"
#include "Overlay.h"

class Player;

// this is just a container
class PlayerHUD : public Overlay
{
public:
	PlayerHUD(unsigned int aPlayerId);
	~PlayerHUD();
};

namespace Database
{
	extern Typed<PlayerHUD *> playerhud;
}
