#pragma once

enum ButtonState
{
	BUTTON_NORMAL = 0,
	BUTTON_SELECTED = 1 << 0,
	BUTTON_ROLLOVER = 1 << 1,
	NUM_BUTTON_STATES = 1 << 2
};

// color typedef (HACK)
typedef Color4 Color4_2[2];

extern const Color4 optionbackcolor[NUM_BUTTON_STATES];
extern const Color4 optionbordercolor[NUM_BUTTON_STATES];
extern const Color4_2 optionlabelcolor[NUM_BUTTON_STATES];
extern const Color4 inertbordercolor[NUM_BUTTON_STATES];
extern const Color4_2 inertlabelcolor[NUM_BUTTON_STATES];

// shell menu option
struct ShellMenuItem
{
	// option button
	Vector2 mButtonPos;
	Vector2 mButtonSize;
	const Color4 *mButtonColor;

	// option label
	char *mLabel;
	Vector2 mLabelPos;
	Vector2 mLabelJustify;
	Vector2 mCharSize;
	const Color4 *mBorderColor;
	const Color4_2 *mLabelColor;

	// button state
	unsigned int mState;

	// action
	fastdelegate::FastDelegate<void ()> mAction;

	// associated variable
	unsigned int mVariable;
	int mValue;

	// render the button
	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
