#pragma once

#include "Overlay.h"

class PlayerOverlayLives : public Overlay
{
public:
	int cur_lives;

public:
	PlayerOverlayLives(unsigned int aPlayerId);
	~PlayerOverlayLives();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
