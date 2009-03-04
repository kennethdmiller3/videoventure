#include "StdAfx.h"
#include "PlayerOverlayLevel.h"
#include "Player.h"
#include "Resource.h"
#include "Drawlist.h"


// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);

// level indicator position
static const Vector2 levelpos(8 + 128 + 8, 24 + 16);


//
// PLAYER OVERLAY: LEVEL
//

// constructor
PlayerOverlayLevel::PlayerOverlayLevel(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, cur_level(-1)
{
	// allocate level draw list
	level_handle = glGenLists(1);

	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayLevel::Render));
}

// destructor
PlayerOverlayLevel::~PlayerOverlayLevel()
{
	// free level draw list
	glDeleteLists(level_handle, 1);
}

// render
void PlayerOverlayLevel::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;

	// get level resource
	Resource *levelresource = Database::resource.Get(id).Get(0x9b99e7dd /* "level" */);
	if (!levelresource)
		return;
	int new_level = xs_FloorToInt(levelresource->GetValue());

	// if the level has not changed...
	if (new_level == cur_level && !wasreset)
	{
		// call the existing draw list
		glCallList(level_handle);
		return;
	}

	// update level
	cur_level = new_level;

	// start a new draw list list
	glNewList(level_handle, GL_COMPILE_AND_EXECUTE);

	// draw the special level
	glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslatef(levelpos.x, levelpos.y, 0.0f);
	glScalef(4, 4, 1);
	glCallList(Database::drawlist.Get(0x8cdedbba /* "circle16" */));
	glPopMatrix();

	// draw remaining level
	char level[16];
	sprintf(level, "x%d", cur_level);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

	glColor4f(0.4f, 0.5f, 1.0f, 1.0f);

	glBegin(GL_QUADS);
	float w = 8;
	float h = -8;
	float x = levelpos.x + 8;
	float y = levelpos.y - 0.5f * h;
	float z = 0;
	OGLCONSOLE_DrawString(level, x, y, w, h, z);

	glEnd();

	glDisable(GL_TEXTURE_2D);

	glEndList();
}
