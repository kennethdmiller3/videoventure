#pragma once

#include "Overlay.h"

class PlayerOverlayLevel : public Overlay
{
	GLuint level_handle;
	int cur_level;
	float cur_part;

public:
	PlayerOverlayLevel(unsigned int aPlayerId);
	~PlayerOverlayLevel();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
