#include "StdAfx.h"
#include "PlayerOverlayLevel.h"
#include "Player.h"
#include "Resource.h"
#include "Drawlist.h"
#include "Font.h"


// level indicator position
static const Vector2 levelpos(8 + 128 + 8, 24 + 16);

// level gauge
static const Rect<float> levelrect =
{
	8, 34, 128, 8
};
static const Color4 levelcolor[] =
{
	Color4( 0.2f, 0.2f, 0.2f, 0.5f),	// level 0
	Color4( 1.0f, 0.0f, 0.0f, 1.0f),	// level 1
	Color4( 1.0f, 1.0f, 0.0f, 1.0f),	// level 2
	Color4( 0.0f, 1.0f, 0.0f, 1.0f),	// level 3
	Color4( 0.0f, 0.0f, 1.0f, 1.0f),	// level 4
	Color4( 0.7f, 0.0f, 1.0f, 1.0f),	// level 5
	Color4( 1.0f, 0.0f, 1.0f, 1.0f),	// level 6
	Color4( 1.0f, 0.7f, 1.0f, 1.0f),	// level 7
	Color4( 1.0f, 1.0f, 1.0f, 1.0f),	// level 8
	Color4( 1.0f, 1.0f, 1.0f, 1.0f),	// level max
};


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
	if (glIsList(level_handle))
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
	float new_part = levelresource->GetValue() - new_level;

	// if the level has not changed...
	if (new_part == cur_part && new_level == cur_level && glIsList(level_handle))
	{
		// call the existing draw list
		glCallList(level_handle);
		return;
	}

	// update level
	cur_level = new_level;
	cur_part = new_part;

	// start a new draw list list
	glNewList(level_handle, GL_COMPILE_AND_EXECUTE);

	// draw level gauge
	glBegin(GL_QUADS);

	// background
	glColor4fv(levelcolor[cur_level]);
	glVertex2f(levelrect.x, levelrect.y);
	glVertex2f(levelrect.x + levelrect.w, levelrect.y);
	glVertex2f(levelrect.x + levelrect.w, levelrect.y + levelrect.h);
	glVertex2f(levelrect.x, levelrect.y + levelrect.h);

	// fill gauge
	glColor4fv(levelcolor[cur_level+1]);
	glVertex2f(levelrect.x, levelrect.y);
	glVertex2f(levelrect.x + levelrect.w * cur_part, levelrect.y);
	glVertex2f(levelrect.x + levelrect.w * cur_part, levelrect.y + levelrect.h);
	glVertex2f(levelrect.x, levelrect.y + levelrect.h);

	glEnd();

	// draw the level icon
	glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslatef(levelpos.x, levelpos.y, 0.0f);
	glScalef(4, 4, 1);
	glCallList(Database::drawlist.Get(0x8cdedbba /* "circle16" */));
	glPopMatrix();

	// draw level number
	char level[16];
	sprintf(level, "x%d", cur_level);

	FontDrawBegin(sDefaultFontHandle);

	FontDrawColor(Color4(0.4f, 0.5f, 1.0f, 1.0f));

	float w = 8;
	float h = -8;
	float x = levelpos.x + 8;
	float y = levelpos.y - 0.5f * h;
	float z = 0;
	FontDrawString(level, x, y, w, h, z);

	FontDrawEnd();

	glEndList();
}
