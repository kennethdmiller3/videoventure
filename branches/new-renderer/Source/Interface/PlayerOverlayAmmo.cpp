#include "StdAfx.h"
#include "PlayerOverlayAmmo.h"
#include "Player.h"
#include "Resource.h"
#include "Render.h"


// ammo gauge
static const Rect<float> ammorect =
{
	8, 34, 128, 8
};
#ifdef PLAYER_AMMO_FLOAT_COLOR
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
#else
static const unsigned int ammocolor[] =
{
	0x7F4C4C4C,	// level 0
	0xFFFF0000,	// level 1
	0xFFFF00B2,	// level 2
	0xFFFF00FF,	// level 3
	0xFFFFB2FF,	// level 4
	0xFFFFFFFF,	// level 5
	0xFFFFFFFF,	// level max
};
#endif

struct Vertex
{
	Vector3 pos;
#ifdef PLAYER_AMMO_FLOAT_COLOR
	Color4 color;
#else
	unsigned int color;
#endif
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
	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayAmmo::Render));
}

// destructor
PlayerOverlayAmmo::~PlayerOverlayAmmo()
{
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

	// update level
	cur_level = new_level;

	// begin drawing
	UseProgram(0);
	SetAttribFormat(0, 3, GL_FLOAT);
	SetAttribFormat(2, 4, GL_UNSIGNED_BYTE);
	SetWorkFormat((1<<0)|(1<<2));
	SetDrawMode(GL_TRIANGLES);

	int base = GetVertexCount();
	register Vertex * __restrict v = static_cast<Vertex *>(AllocVertices(8));

	// background
	v->pos = Vector3(ammorect.x, ammorect.y, 0);
	v->color = ammocolor[cur_level];
	++v;
	v->pos = Vector3(ammorect.x + ammorect.w, ammorect.y, 0);
	v->color = ammocolor[cur_level];
	++v;
	v->pos = Vector3(ammorect.x + ammorect.w, ammorect.y + ammorect.h, 0);
	v->color = ammocolor[cur_level];
	++v;
	v->pos = Vector3(ammorect.x, ammorect.y + ammorect.h, 0);
	v->color = ammocolor[cur_level];
	++v;

	// fill gauge
	v->pos = Vector3(ammorect.x, ammorect.y, 0);
	v->color = ammocolor[cur_level+1];
	++v;
	v->pos = Vector3(ammorect.x + ammorect.w * cur_ammo, ammorect.y, 0);
	v->color = ammocolor[cur_level+1];
	++v;
	v->pos = Vector3(ammorect.x + ammorect.w * cur_ammo, ammorect.y + ammorect.h, 0);
	v->color = ammocolor[cur_level+1];
	++v;
	v->pos = Vector3(ammorect.x, ammorect.y + ammorect.h, 0);
	v->color = ammocolor[cur_level+1];
	++v;

	// indices
	IndexQuads(base, GetVertexCount() - base);
}
