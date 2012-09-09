#include "StdAfx.h"
#include "ShellMenuItem.h"
#include "VarItem.h"
#include "Font.h"
#include "Drawlist.h"
#include "Render.h"

// color palette
const Color4 optionbackcolor[NUM_BUTTON_STATES] =
{
	Color4( 0.2f, 0.2f, 0.2f, 0.5f ),
	Color4( 0.1f, 0.3f, 1.0f, 0.5f ),
	Color4( 0.4f, 0.4f, 0.4f, 0.5f ),
	Color4( 0.1f, 0.7f, 1.0f, 0.5f ),
};
const Color4 optionbordercolor[NUM_BUTTON_STATES] =
{
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
};
const Color4_2 optionlabelcolor[NUM_BUTTON_STATES] =
{
	{ Color4( 0.1f, 0.6f, 1.0f, 1.0f ), Color4( 0.1f, 0.6f, 1.0f, 1.0f ) },
	{ Color4( 1.0f, 0.9f, 0.1f, 1.0f ), Color4( 1.0f, 0.9f, 0.1f, 1.0f ) },
	{ Color4( 0.7f, 0.7f, 0.7f, 1.0f ), Color4( 1.0f, 1.0f, 1.0f, 1.0f ) },
	{ Color4( 1.0f, 0.9f, 0.1f, 1.0f ), Color4( 1.0f, 1.0f, 1.0f, 1.0f ) },
};
const Color4 inertbordercolor[] =
{
	Color4( 0.1f, 0.1f, 0.1f, 1.0f ),
};
const Color4_2 inertlabelcolor[] =
{
	{ Color4( 0.7f, 0.7f, 0.7f, 1.0f ), Color4( 0.7f, 0.7f, 0.7f, 1.0f ) }
};

struct Vertex
{
	Vector3 pos;
#ifdef SHELL_MENU_FLOAT_COLOR
	Color4 color;
#else
	unsigned int color;
#endif
};

// render the button
void ShellMenuItem::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	unsigned int state = mState;
	if (VarItem *item = Database::varitem.Get(mVariable))
		if (item->GetInteger() == mValue)
			state |= BUTTON_SELECTED;

	if (mButtonColor)
	{
		// render button
		SetWorkFormat((1<<0)|(1<<2));
		SetDrawMode(GL_TRIANGLES);

		size_t base = GetVertexCount();
		unsigned int color = 
			GLubyte(Clamp(xs_RoundToInt(mButtonColor[state].r * 255), 0, 255)) |
			GLubyte(Clamp(xs_RoundToInt(mButtonColor[state].g * 255), 0, 255)) << 8 |
			GLubyte(Clamp(xs_RoundToInt(mButtonColor[state].b * 255), 0, 255)) << 16 |
			GLubyte(Clamp(xs_RoundToInt(mButtonColor[state].a * 255), 0, 255)) << 24;
		register Vertex * __restrict v = static_cast<Vertex *>(AllocVertices(4));
		v->pos = Vector3(mButtonPos.x, mButtonPos.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(mButtonPos.x + mButtonSize.x, mButtonPos.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(mButtonPos.x + mButtonSize.x, mButtonPos.y + mButtonSize.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(mButtonPos.x, mButtonPos.y + mButtonSize.y, 0);
		v->color = color;
		++v;
		IndexQuads(base, GetVertexCount() - base);
	}

	if (mLabel)
	{
		FontDrawBegin(sDefaultFontHandle);

		// get text corner position
		size_t labellen = strlen(mLabel);
		Vector2 labelcorner(
			mButtonPos.x + mLabelPos.x - mLabelJustify.x * mCharSize.x * labellen,
			mButtonPos.y + mLabelPos.y + (1.0f - mLabelJustify.y) * mCharSize.y);

		if (mBorderColor)
		{
			// render border
			FontDrawColor(mBorderColor[state]);
			FontDrawString(mLabel, labelcorner.x - 2, labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
			FontDrawString(mLabel, labelcorner.x    , labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
			FontDrawString(mLabel, labelcorner.x + 2, labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
			FontDrawString(mLabel, labelcorner.x - 2, labelcorner.y    , mCharSize.x, -mCharSize.y, 0);
			FontDrawString(mLabel, labelcorner.x + 2, labelcorner.y    , mCharSize.x, -mCharSize.y, 0);
			FontDrawString(mLabel, labelcorner.x - 2, labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
			FontDrawString(mLabel, labelcorner.x    , labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
			FontDrawString(mLabel, labelcorner.x + 2, labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
		}

		// render label
		Color4 color;
		float interp = ((sim_turn & 16) ? 16 - (sim_turn & 15) : (sim_turn & 15)) / 16.0f;
		for (int c = 0; c < 4; c++)
			color[c] = Lerp(mLabelColor[state][0][c], mLabelColor[state][1][c], interp);
		FontDrawColor(color);
		FontDrawString(mLabel, labelcorner.x, labelcorner.y, mCharSize.x, -mCharSize.y, 0);

		FontDrawEnd();
	}
}
