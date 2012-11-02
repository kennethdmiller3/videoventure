#include "StdAfx.h"
#include "PlayerOverlaySpecial.h"
#include "Player.h"
#include "Resource.h"
#include "Drawlist.h"
#include "Font.h"
#include "Render.h"
#include "MatrixStack.h"
#include "Expression.h"
#include "ExpressionEntity.h"
#include "ShaderColor.h"


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
	Overlay::SetAction(Overlay::Action(this, &PlayerOverlaySpecial::Render));

	const std::vector<unsigned int> &drawlist = Database::drawlist.Get(0x8cdedbba /* "circle16" */);
	if (drawlist.size())
	{
		// create drawlist for icon
		Expression::Append(icon_drawlist, DO_AttribValue, ShaderColor::gAttribColor);
		Expression::Append(icon_drawlist, Expression::Read<__m128>, 0.4f,  0.5f, 1.0f, 1.0f);
		Expression::Append(icon_drawlist, DO_PushMatrix);
		Expression::Append(icon_drawlist, DO_Translate, Expression::Read<__m128>, specialpos.x, specialpos.y, 0.0f, 1.0f);
		Expression::Append(icon_drawlist, DO_Scale, Expression::Read<__m128>, 4.0f, 4.0f, 1.0f, 1.0f);
		icon_drawlist.insert(icon_drawlist.end(), drawlist.begin(), drawlist.end());
		Expression::Append(icon_drawlist, DO_PopMatrix);
	}
}

// destructor
PlayerOverlaySpecial::~PlayerOverlaySpecial()
{
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

	// update special
	cur_special = new_special;

	// draw the special ammo icon
	EntityContext context(&icon_drawlist[0], icon_drawlist.size(), 0.0f, aId);
	while (context.mStream < context.mEnd)
		Expression::Evaluate<void>(context);

	// draw remaining special ammo
	char special[16];
	sprintf(special, "x%d", cur_special);

	FontDrawBegin(sDefaultFontHandle);

	FontDrawColor(Color4(0.4f, 0.5f, 1.0f, 1.0f));

	float w = 8;
	float h = -8;
	float x = specialpos.x + 8;
	float y = specialpos.y - 0.5f * h;
	float z = 0;
	FontDrawString(special, x, y, w, h, z);

	FontDrawEnd();
}
