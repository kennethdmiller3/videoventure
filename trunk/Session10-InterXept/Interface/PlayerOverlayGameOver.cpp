#include "StdAfx.h"
#include "PlayerOverlayGameOver.h"
#include "Player.h"
#include "Controller.h"
#include "GameState.h"	// for setgamestate


// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);


//
// PLAYER OVERLAY: GAME OVER
//

// constructor
PlayerOverlayGameOver::PlayerOverlayGameOver(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, gameovertimer(0.0f)
{
	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayGameOver::Render));
}

// destructor
PlayerOverlayGameOver::~PlayerOverlayGameOver()
{
}

// render
void PlayerOverlayGameOver::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;

	// if tracking an active controller...
	Controller *controller = Database::controller.Get(id);
	if (controller)
	{
		gameovertimer = 0.0f;
	}
	else if (player->mLives <= 0 && player->mTimer <= 0.0f)
	{
		// display game over
		gameovertimer += frame_time;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);


		glBegin(GL_QUADS);

		float lerp = std::min(gameovertimer, 1.0f);
		static char *text = "GAME OVER";
		const float w = Lerp<float>(96, 48, lerp);
		const float h = Lerp<float>(-64, -32, lerp);
		const float x = 320 - 0.5f * w * strlen(text);
		const float y = 240 - 0.5f * h;
		const float z = 0;
		const float a = lerp * lerp;

		glColor4f(0.1f, 0.1f, 0.1f, a);
		OGLCONSOLE_DrawString(text, x - 2, y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x    , y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x - 2, y    , w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y    , w, h, z);
		OGLCONSOLE_DrawString(text, x - 2, y + 2, w, h, z);
		OGLCONSOLE_DrawString(text, x    , y + 2, w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y + 2, w, h, z);

		glColor4f(1.0f, 0.2f, 0.1f, a);
		OGLCONSOLE_DrawString(text, x, y, w, h, z);

		glEnd();

		glDisable(GL_TEXTURE_2D);

		// HACK: this should not be done right nere
		if (gameovertimer > 5)
			setgamestate = STATE_SHELL;
	}
}
