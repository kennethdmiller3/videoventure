#pragma once

#include "Overlay.h"

class PlayerOverlayAmmo : public Overlay
{
	float cur_ammo;
	int cur_level;

public:
	PlayerOverlayAmmo(unsigned int aPlayerId);
	~PlayerOverlayAmmo();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
