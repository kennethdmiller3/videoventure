#pragma once

#include "Overlay.h"

class PlayerOverlaySpecial : public Overlay
{
	int cur_special;
	std::vector<unsigned int> icon_drawlist;

public:
	PlayerOverlaySpecial(unsigned int aPlayerId);
	~PlayerOverlaySpecial();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
