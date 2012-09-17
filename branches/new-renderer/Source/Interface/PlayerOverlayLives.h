#pragma once

#include "Overlay.h"

class PlayerOverlayLives : public Overlay
{
public:
	int cur_lives;
	std::vector<unsigned int> icon_drawlist;

public:
	PlayerOverlayLives(unsigned int aPlayerId);
	~PlayerOverlayLives();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
