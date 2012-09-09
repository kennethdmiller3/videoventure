#include "StdAfx.h"
#include "PlayerOverlayLives.h"
#include "Player.h"
#include "Drawlist.h"
#include "Font.h"


// lives indicator position
static const Vector2 livespos(8 + 128 + 8, 24 + 8 / 2);


//
// PLAYER OVERLAY: LIVES
//

// constructor
PlayerOverlayLives::PlayerOverlayLives(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, cur_lives(-1)
{
	// allocate lives draw list
	lives_handle = glGenLists(1);

	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayLives::Render));
}

// destructor
PlayerOverlayLives::~PlayerOverlayLives()
{
	// free lives draw list
	if (glIsList(lives_handle))
		glDeleteLists(lives_handle, 1);
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

	// if the lives count has not changed...
	if (new_lives == cur_lives && glIsList(lives_handle))
	{
		// call the existing draw list
		glCallList(lives_handle);
		return;
	}

	// update lives
	cur_lives = new_lives;

	// start a new draw list list
	glNewList(lives_handle, GL_COMPILE_AND_EXECUTE);

	// draw the player ship
	glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslatef(livespos.x, livespos.y, 0.0f);
	glScalef(-0.5f, -0.5f, 1);
	glCallList(Database::drawlist.Get(0xeec1dafa /* "playership" */));
	glPopMatrix();

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

	glEndList();
}
