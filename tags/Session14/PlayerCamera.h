#pragma once

#include "Updatable.h"

class PlayerCamera : public Updatable
{
public:
	// camera values
	unsigned int mTrackId;
	Vector2 mTrackPos0;
	Vector2 mTrackPos1;
	Vector2 mTrackAim;

public:
	PlayerCamera(unsigned int aPlayerId);
	~PlayerCamera();

	void Update(float aStep);
};


namespace Database
{
	extern Typed<PlayerCamera *> playercamera;
}
