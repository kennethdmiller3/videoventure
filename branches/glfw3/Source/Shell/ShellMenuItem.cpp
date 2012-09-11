#include "StdAfx.h"
#include "ShellMenuItem.h"
#include "VarItem.h"
#include "Font.h"

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
		glBegin(GL_QUADS);
		glColor4fv(mButtonColor[state]);
		glVertex2f(mButtonPos.x, mButtonPos.y);
		glVertex2f(mButtonPos.x + mButtonSize.x, mButtonPos.y);
		glVertex2f(mButtonPos.x + mButtonSize.x, mButtonPos.y + mButtonSize.y);
		glVertex2f(mButtonPos.x, mButtonPos.y + mButtonSize.y);
		glEnd();
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
