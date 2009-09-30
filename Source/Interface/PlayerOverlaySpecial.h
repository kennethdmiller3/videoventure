#pragma once

#include "Overlay.h"

class PlayerOverlaySpecial : public Overlay
{
	GLuint special_handle;
	int cur_special;

public:
	PlayerOverlaySpecial(unsigned int aPlayerId);
	~PlayerOverlaySpecial();

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
