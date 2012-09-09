#include "StdAfx.h"
#include "PlayerOverlayLives.h"
#include "Player.h"
#include "Drawlist.h"
#include "Font.h"
#include "Render.h"
#include "MatrixStack.h"


// lives indicator position
static const Vector2 livespos(8 + 128 + 8, 24 + 8 / 2);


//
// PLAYER OVERLAY: LIVES
//

// constructor
PlayerOverlayLives::PlayerOverlayLives(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
{
	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayLives::Render));
}

// destructor
PlayerOverlayLives::~PlayerOverlayLives()
{
}

// render
void PlayerOverlayLives::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get player lives count
	int new_lives = player->mLives;
	if (new_lives == INT_MAX)
		return;

	// update lives
	cur_lives = new_lives;

	// draw the player ship
	SetAttribConstant(2, _mm_setr_ps(0.4f, 0.5f, 1.0f, 1.0f));
	StackPush();
	StackTranslate(_mm_setr_ps(livespos.x, livespos.y, 0.0f, 1.0f));
	StackScale(_mm_setr_ps(-0.5f, -0.5f, 1.0f, 1.0f));
	RenderStaticDrawlist(0xeec1dafa /* "playership" */, 0.0f, Transform2::Identity());
	StackPop();

	// draw remaining lives
	char lives[16];
	sprintf(lives, "x%d", cur_lives);

	FontDrawBegin(sDefaultFontHandle);

	FontDrawColor(Color4(0.4f, 0.5f, 1.0f, 1.0f));
	float w = 8;
	float h = -8;
	float x = livespos.x + 8;
	float y = livespos.y - 0.5f * h;
	float z = 0;
	FontDrawString(lives, x, y, w, h, z);

	FontDrawEnd();
}
