#include "StdAfx.h"
#include "PlayerOverlayLives.h"
#include "Player.h"
#include "Drawlist.h"
#include "Font.h"
#include "Render.h"
#include "MatrixStack.h"
#include "Expression.h"
#include "ExpressionEntity.h"
#include "ShaderColor.h"


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

	const std::vector<unsigned int> &drawlist = Database::drawlist.Get(0xeec1dafa /* "playership" */);
	if (drawlist.size())
	{
		// create drawlist for icon
		Expression::Append(icon_drawlist, DO_AttribValue, ATTRIB_INDEX_COLOR);
		Expression::Append(icon_drawlist, Expression::Read<__m128>, 0.4f,  0.5f, 1.0f, 1.0f);
		Expression::Append(icon_drawlist, DO_PushMatrix);
		Expression::Append(icon_drawlist, DO_Translate, Expression::Read<__m128>, livespos.x, livespos.y, 0.0f, 1.0f);
		Expression::Append(icon_drawlist, DO_Scale, Expression::Read<__m128>, -0.5f, -0.5f, 1.0f, 1.0f);
		icon_drawlist.insert(icon_drawlist.end(), drawlist.begin(), drawlist.end());
		Expression::Append(icon_drawlist, DO_PopMatrix);
	}
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

	// draw icon
	EntityContext context(&icon_drawlist[0], icon_drawlist.size(), 0.0f, aId);
	while (context.mStream < context.mEnd)
		Expression::Evaluate<void>(context);

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
