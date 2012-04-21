#include "StdAfx.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "Sound.h"


// draw options
void ShellMenu::RenderOptions(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// cursor position
	float cursor_x = 320 - 240 * input.value[Input::MENU_HORIZONTAL];
	float cursor_y = 240 - 240 * input.value[Input::MENU_VERTICAL];

	// HACK use the main page
	ShellMenuPage &page = *mActive;

	// for each option on the page...
	for (unsigned int i = 0; i < page.mCount; ++i)
	{
		// get the option
		ShellMenuItem &option = page.mOption[i];

		if (option.mAction)
		{
			// on mouse rollover
			if (cursor_x >= option.mButtonPos.x && cursor_x <= option.mButtonPos.x + option.mButtonSize.x &&
				cursor_y >= option.mButtonPos.y && cursor_y <= option.mButtonPos.y + option.mButtonSize.y)
			{
				// play a sound if not rolled over
				if (!(option.mState & BUTTON_ROLLOVER))
					PlaySoundCue(0, 0x5d147744 /* "rollover" */);

				// mark as rollover
				option.mState |= BUTTON_ROLLOVER;

				// if mouse button pressed...
				if (input.value[Input::MENU_CLICK])
				{
					// play a sound if not selected
					if (!(option.mState & BUTTON_SELECTED))
						PlaySoundCue(0, 0x5c7ea86f /* "click" */);

					// mark as selected
					option.mState |= BUTTON_SELECTED;
				}
				else if (option.mState & BUTTON_SELECTED)
				{
					// mark as not selected
					option.mState &= ~BUTTON_SELECTED;

					// perform action
					(option.mAction)();
				}
			}
			else
			{
				// mark as not rollover
				option.mState &= ~BUTTON_ROLLOVER;

				if (!input.value[Input::MENU_CLICK])
				{
					// mark as not selected
					option.mState &= ~BUTTON_SELECTED;
				}
			}
		}

		// render the option
		option.Render(aId, aTime, aTransform);
	}

	// draw reticule (HACK)
	glPushMatrix();
	glTranslatef(cursor_x, cursor_y, 0.0f);
	glCallList(reticule_handle);
	glPopMatrix();
}
