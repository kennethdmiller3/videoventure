#pragma once

#include "Updatable.h"
#include "Overlay.h"

class Player;

class PlayerHUD : public Updatable, public Overlay
{
public:
	// drawlists
	GLuint score_handle;
	GLuint lives_handle;
	GLuint ammo_handle;

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

	// reticule values
	Vector2 aimpos[2];

public:
	PlayerHUD(unsigned int aPlayerId);
	~PlayerHUD();

	void Update(float aStep);
	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);

protected:
	void RenderScore(const Player *player);
	void RenderHealth(const Player *player);
	void RenderLives(const Player *player);
	void RenderAmmo(const Player *player);
	void RenderLevel(const Player *player);
	void RenderSpecial(const Player *player);
	void RenderGameOver(const Player *player);
};


namespace Database
{
	extern Typed<PlayerHUD *> playerhud;
}
