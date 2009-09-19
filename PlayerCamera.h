#pragma once

#include "Updatable.h"

class PlayerCamera : public Updatable
{
public:
	// camera values
	unsigned int trackid;
	Vector2 trackpos[2];
	Vector2 trackaim;

public:
	PlayerCamera(unsigned int aPlayerId);
	~PlayerCamera();

	void Update(float aStep);
};


namespace Database
{
	extern Typed<PlayerCamera *> playercamera;
}
