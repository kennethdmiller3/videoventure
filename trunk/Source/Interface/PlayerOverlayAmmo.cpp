#include "StdAfx.h"
#include "PlayerOverlayAmmo.h"
#include "Player.h"
#include "Resource.h"


// ammo gauge
static const Rect<float> ammorect =
{
	8, 34, 128, 8
};
static const Color4 ammocolor[] =
{
	Color4( 0.3f, 0.3f, 0.3f, 0.5f),	// level 0
	Color4( 0.0f, 0.0f, 1.0f, 1.0f),	// level 1
	Color4( 0.7f, 0.0f, 1.0f, 1.0f),	// level 2
	Color4( 1.0f, 0.0f, 1.0f, 1.0f),	// level 3
	Color4( 1.0f, 0.7f, 1.0f, 1.0f),	// level 4
	Color4( 1.0f, 1.0f, 1.0f, 1.0f),	// level 5
	Color4( 1.0f, 1.0f, 1.0f, 1.0f),	// level max
};


//
// PLAYER OVERLAY: AMMO
//

// constructor
PlayerOverlayAmmo::PlayerOverlayAmmo(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, cur_ammo(-FLT_MAX)
	, cur_level(-1)
{
	// allocate ammo draw list
	ammo_handle = glGenLists(1);

	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayAmmo::Render));
}

// destructor
PlayerOverlayAmmo::~PlayerOverlayAmmo()
{
	// free ammo draw list
	if (glIsList(ammo_handle))
		glDeleteLists(ammo_handle, 1);
}

// render
void PlayerOverlayAmmo::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;

	// draw player ammo (HACK)
	Resource *ammoresource = Database::resource.Get(id).Get(0x5b9b0daf /* "ammo" */);
	if (!ammoresource)
		return;

	// get ammo ratio
	float new_ammo = 0.0f;
	const ResourceTemplate &ammoresourcetemplate = Database::resourcetemplate.Get(id).Get(0x5b9b0daf /* "ammo" */);
	if (ammoresourcetemplate.mMaximum > 0)
	{
		new_ammo = ammoresource->GetValue() / ammoresourcetemplate.mMaximum;
	}
	
	// get level
	int new_level = 1;
	Resource *levelresource = Database::resource.Get(id).Get(0x9b99e7dd /* "level" */);
	if (levelresource)
	{
		new_level = xs_FloorToInt(levelresource->GetValue());
	}

	// if the lives count has not changed...
	if (new_ammo == cur_ammo && new_level == cur_level && glIsList(ammo_handle))
	{
		// call the existing draw list
		glCallList(ammo_handle);
		return;
	}

	// update ammo
	cur_ammo = new_ammo;
	cur_level = new_level;

	// start a new draw list list
	glNewList(ammo_handle, GL_COMPILE_AND_EXECUTE);

	glBegin(GL_QUADS);

	// background
	glColor4fv(ammocolor[cur_level]);
	glVertex2f(ammorect.x, ammorect.y);
	glVertex2f(ammorect.x + ammorect.w, ammorect.y);
	glVertex2f(ammorect.x + ammorect.w, ammorect.y + ammorect.h);
	glVertex2f(ammorect.x, ammorect.y + ammorect.h);

	// fill gauge
	glColor4fv(ammocolor[cur_level+1]);
	glVertex2f(ammorect.x, ammorect.y);
	glVertex2f(ammorect.x + ammorect.w * cur_ammo, ammorect.y);
	glVertex2f(ammorect.x + ammorect.w * cur_ammo, ammorect.y + ammorect.h);
	glVertex2f(ammorect.x, ammorect.y + ammorect.h);

	glEnd();

	glEndList();
}
