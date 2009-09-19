#pragma once

#include "Overlay.h"

class PlayerOverlayGameOver : public Overlay
{
public:
	float gameovertimer;

public:
	PlayerOverlayGameOver(unsigned int aPlayerId);
	~PlayerOverlayGameOver();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
