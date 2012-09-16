#include "StdAfx.h"
#include "PlayerOverlayLevel.h"
#include "Player.h"
#include "Resource.h"
#include "Drawlist.h"
#include "Font.h"
#include "Render.h"
#include "MatrixStack.h"


// level indicator position
static const Vector2 levelpos(8 + 128 + 8, 24 + 16);

// level gauge
static const Rect<float> levelrect =
{
	8, 34, 128, 8
};
#ifdef PLAYER_LEVEL_FLOAT_COLOR
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
#else
static const unsigned int levelcolor[] =
{
	0x7F333333,	// level 0
	0xFF0000FF,	// level 1
	0xFF00FFFF,	// level 2
	0xFF00FF00,	// level 3
	0xFFFF0000,	// level 4
	0xFFFF00B2,	// level 5
	0xFFFF00FF,	// level 6
	0xFFFFB2FF,	// level 7
	0xFFFFFFFF,	// level 8
	0xFFFFFFFF,	// level max
};
#endif

struct Vertex
{
	Vector3 pos;
#ifdef PLAYER_LEVEL_FLOAT_COLOR
	Color4 color;
#else
	unsigned int color;
#endif
};


//
// PLAYER OVERLAY: LEVEL
//

// constructor
PlayerOverlayLevel::PlayerOverlayLevel(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, cur_level(-1)
{
	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayLevel::Render));
}

// destructor
PlayerOverlayLevel::~PlayerOverlayLevel()
{
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

	// update level
	cur_level = new_level;
	cur_part = new_part;

	// draw level gauge

	// begin drawing
	UseProgram(0);
	SetAttribFormat(0, 3, GL_FLOAT);
	SetAttribFormat(2, 4, GL_UNSIGNED_BYTE);
	SetWorkFormat((1<<0)|(1<<2));
	SetDrawMode(GL_TRIANGLES);

	int base = GetVertexCount();
	register Vertex * __restrict v = static_cast<Vertex *>(AllocVertices(8));

	// background
	v->pos = Vector3(levelrect.x, levelrect.y, 0);
	v->color = levelcolor[cur_level];
	++v;
	v->pos = Vector3(levelrect.x + levelrect.w, levelrect.y, 0);
	v->color = levelcolor[cur_level];
	++v;
	v->pos = Vector3(levelrect.x + levelrect.w, levelrect.y + levelrect.h, 0);
	v->color = levelcolor[cur_level];
	++v;
	v->pos = Vector3(levelrect.x, levelrect.y + levelrect.h, 0);
	v->color = levelcolor[cur_level];
	++v;

	// fill gauge
	v->pos = Vector3(levelrect.x, levelrect.y, 0);
	v->color = levelcolor[cur_level+1];
	++v;
	v->pos = Vector3(levelrect.x + levelrect.w * cur_part, levelrect.y, 0);
	v->color = levelcolor[cur_level+1];
	++v;
	v->pos = Vector3(levelrect.x + levelrect.w * cur_part, levelrect.y + levelrect.h, 0);
	v->color = levelcolor[cur_level+1];
	++v;
	v->pos = Vector3(levelrect.x, levelrect.y + levelrect.h, 0);
	v->color = levelcolor[cur_level+1];
	++v;

	// indices
	IndexQuads(base, GetVertexCount() - base);

	// draw the level icon
	SetAttribConstant(2, _mm_setr_ps(0.4f, 0.5f, 1.0f, 1.0f));
	StackPush();
	StackTranslate(_mm_setr_ps(levelpos.x, levelpos.y, 0.0f, 1.0f));
	StackScale(_mm_setr_ps(4.0f, 4.0f, 1.0f, 1.0f));
	RenderStaticDrawlist(0x8cdedbba /* "circle16" */, 0.0f, Transform2::Identity());
	StackPop();

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
}
