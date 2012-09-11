#pragma once

#include "Overlay.h"
#include "Updatable.h"

class PlayerOverlayReticule : public Updatable, public Overlay
{
public:
	// reticule values
	Vector2 aimpos[2];
	int showreticule;

public:
	PlayerOverlayReticule(unsigned int aPlayerId);
	~PlayerOverlayReticule();

	void Update(float aStep);
	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
