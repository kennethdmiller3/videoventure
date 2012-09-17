#pragma once

#include "Overlay.h"

class PlayerOverlayScore : public Overlay
{
public:
	int cur_score;

public:
	PlayerOverlayScore(unsigned int aPlayerId);
	~PlayerOverlayScore();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
