#pragma once

#include "Overlay.h"

class PlayerOverlayLevel : public Overlay
{
	int cur_level;
	float cur_part;
	std::vector<unsigned int> icon_drawlist;

public:
	PlayerOverlayLevel(unsigned int aPlayerId);
	~PlayerOverlayLevel();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
