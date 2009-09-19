#pragma once

#include "Overlay.h"

class PlayerOverlayHealth : public Overlay
{
public:
	// fill values
	float fill;

	// drain values
	float drain;
	float draindelay;

	// flash values
	struct Flash
	{
		float left;
		float right;
		float fade;
	};
	Flash flash[16];
	int flashcount;

	// pulse values
	float pulsetimer;

public:
	PlayerOverlayHealth(unsigned int aPlayerId);
	~PlayerOverlayHealth();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
