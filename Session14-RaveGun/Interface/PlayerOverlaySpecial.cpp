#include "StdAfx.h"
#include "PlayerOverlaySpecial.h"
#include "Player.h"
#include "Resource.h"
#include "Drawlist.h"


// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);

// special ammo position
static const Vector2 specialpos(8 + 128 + 8, 24 + 16);


//
// PLAYER OVERLAY: SPECIAL
//

// constructor
PlayerOverlaySpecial::PlayerOverlaySpecial(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, cur_special(-1)
{
	// allocate special draw list
	special_handle = glGenLists(1);

	Overlay::SetAction(Overlay::Action(this, &PlayerOverlaySpecial::Render));
}

// destructor
PlayerOverlaySpecial::~PlayerOverlaySpecial()
{
	// free special draw list
	glDeleteLists(special_handle, 1);
}

// render
void PlayerOverlaySpecial::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the player entity (HACK)
	unsigned int id = player->GetId();

	// get "special" ammo resource (HACK)
	Resource *specialresource = Database::resource.Get(id).Get(0xd940d530 /* "special" */);
	if (!specialresource)
		return;
	int new_special = xs_FloorToInt(specialresource->GetValue());

	// if the special has not changed...
	if (new_special == cur_special && !wasreset)
	{
		// call the existing draw list
		glCallList(special_handle);
		return;
	}

	// update special
	cur_special = new_special;

	// start a new draw list list
	glNewList(special_handle, GL_COMPILE_AND_EXECUTE);

	// draw the special ammo icon
	glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslatef(specialpos.x, specialpos.y, 0.0f);
	glScalef(4, 4, 1);
	glCallList(Database::drawlist.Get(0x8cdedbba /* "circle16" */));
	glPopMatrix();

	// draw remaining special ammo
	char special[16];
	sprintf(special, "x%d", cur_special);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

	glColor4f(0.4f, 0.5f, 1.0f, 1.0f);

	glBegin(GL_QUADS);
	float w = 8;
	float h = -8;
	float x = specialpos.x + 8;
	float y = specialpos.y - 0.5f * h;
	float z = 0;
	OGLCONSOLE_DrawString(special, x, y, w, h, z);

	glEnd();

	glDisable(GL_TEXTURE_2D);

	glEndList();
}
