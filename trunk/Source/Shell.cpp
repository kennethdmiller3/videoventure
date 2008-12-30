#include "StdAfx.h"

#include "Shell.h"
#include "GameState.h"
#include "Variable.h"
#include "oglconsole.h"
#include "Title.h"
#include "Sound.h"
#include "Collidable.h"
#include "PlayerHUD.h"
#include "Preferences.h"


// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_CreateFont();
extern "C" void OGLCONSOLE_Resize(OGLCONSOLE_Console console);

extern bool escape;

extern void InitWindowAction(void);

extern unsigned int reticule_handle;

extern bool InitInput(const char *config);
extern bool InitLevel(const char *config);

enum ButtonState
{
	BUTTON_NORMAL = 0,
	BUTTON_SELECTED = 1 << 0,
	BUTTON_ROLLOVER = 1 << 1,
	NUM_BUTTON_STATES = 1 << 2
};

static const Color4 optionbackcolor[NUM_BUTTON_STATES] =
{
	Color4( 0.2f, 0.2f, 0.2f, 0.5f ),
	Color4( 0.1f, 0.3f, 1.0f, 0.5f ),
	Color4( 0.4f, 0.4f, 0.4f, 0.5f ),
	Color4( 0.1f, 0.7f, 1.0f, 0.5f ),
};
static const Color4 optionbordercolor[NUM_BUTTON_STATES] =
{
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
	Color4( 0.0f, 0.0f, 0.0f, 1.0f ),
};
static const Color4_2 optionlabelcolor[NUM_BUTTON_STATES] =
{
	{ Color4( 0.1f, 0.6f, 1.0f, 1.0f ), Color4( 0.1f, 0.6f, 1.0f, 1.0f ) },
	{ Color4( 1.0f, 0.9f, 0.1f, 1.0f ), Color4( 1.0f, 0.9f, 0.1f, 1.0f ) },
	{ Color4( 0.7f, 0.7f, 0.7f, 1.0f ), Color4( 1.0f, 1.0f, 1.0f, 1.0f ) },
	{ Color4( 1.0f, 0.9f, 0.1f, 1.0f ), Color4( 1.0f, 1.0f, 1.0f, 1.0f ) },
};
static const Color4 inertbordercolor[] =
{
	Color4( 0.1f, 0.1f, 0.1f, 1.0f ),
};
static const Color4_2 inertlabelcolor[] =
{
	{ Color4( 0.7f, 0.7f, 0.7f, 1.0f ), Color4( 0.7f, 0.7f, 0.7f, 1.0f ) }
};

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
	void Render(unsigned int aId, float aTime, const Transform2 &aTransform)
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
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

			glBegin(GL_QUADS);

			// get text corner position
			size_t labellen = strlen(mLabel);
			Vector2 labelcorner(
				mButtonPos.x + mLabelPos.x - mLabelJustify.x * mCharSize.x * labellen,
				mButtonPos.y + mLabelPos.y + (1.0f - mLabelJustify.y) * mCharSize.y);

			if (mBorderColor)
			{
				// render border
				glColor4fv(mBorderColor[state]);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x - 2, labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x    , labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x + 2, labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x - 2, labelcorner.y    , mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x + 2, labelcorner.y    , mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x - 2, labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x    , labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x + 2, labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
			}

			// render label
			Color4 color;
			float interp = ((sim_turn & 16) ? 16 - (sim_turn & 15) : (sim_turn & 15)) / 16.0f;
			for (int c = 0; c < 4; c++)
				color[c] = Lerp(mLabelColor[state][0][c], mLabelColor[state][1][c], interp);
			glColor4fv(color);
			OGLCONSOLE_DrawString(mLabel, labelcorner.x, labelcorner.y, mCharSize.x, -mCharSize.y, 0);

			glEnd();

			glDisable(GL_TEXTURE_2D);
		}
	}
};

// shell menu page
struct ShellMenuPage
{
	ShellMenuItem *mOption;
	unsigned int mCount;

	fastdelegate::FastDelegate<void ()> mEnter;
	fastdelegate::FastDelegate<void ()> mExit;

	ShellMenuPage *mParent;
};

// shell menu
struct ShellMenu
{
	ShellMenuPage *mActive;

	void Push(ShellMenuPage *aPage)
	{
		aPage->mParent = mActive;
		if (mActive && mActive->mExit)
			(mActive->mExit)();
		mActive = aPage;
		if (mActive && mActive->mEnter)
			(mActive->mEnter)();
	}
	void Pop()
	{
		if (mActive && mActive->mExit)
			(mActive->mExit)();
		mActive = mActive->mParent;
		if (mActive && mActive->mEnter)
			(mActive->mEnter)();
	}
};

// forward declaration
extern ShellMenu shellmenu;
extern ShellMenuPage shellmenumainpage;
extern ShellMenuPage shellmenuoptionspage;
extern ShellMenuPage shellmenuvideopage;
extern ShellMenuPage shellmenuaudiopage;

//
// MAIN MENU

// start item
void ShellMenuMainPressStart(void)
{
	setgamestate = STATE_PLAY;
}

void ShellMenuMainPressOptions(void)
{
	shellmenu.Push(&shellmenuoptionspage);
}

void ShellMenuMainPressQuit(void)
{
	setgamestate = STATE_QUIT;
}

ShellMenuItem shellmenumainitems[] =
{
	{
		Vector2( 320 - 160, 200 + 80 * 0 ),
		Vector2( 320, 64 ),
		optionbackcolor,
		"START",
		Vector2(160, 32),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressStart,
	},
	{
		Vector2( 320 - 160, 200 + 80 * 1 ),
		Vector2( 320, 64 ),
		optionbackcolor,
		"OPTIONS",
		Vector2(160, 32),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressOptions,
	},
	{
		Vector2( 320 - 160, 200 + 80 * 2 ),
		Vector2( 320, 64 ),
		optionbackcolor,
		"QUIT",
		Vector2(160, 32),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressQuit,
	},
};

//
// OPTION MENU

void ShellMenuOptionsPressVideo()
{
	shellmenu.Push(&shellmenuvideopage);
}

void ShellMenuOptionsPressAudio()
{
	shellmenu.Push(&shellmenuaudiopage);
}

void ShellMenuOptionsPressDebug()
{
}

void ShellMenuOptionsPressBack()
{
	shellmenu.Pop();
}

ShellMenuItem shellmenuoptionsitems[] =
{
	{
		Vector2( 320 - 160, 200 + 64 * 0 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"VIDEO",
		Vector2( 160, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressVideo,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 1 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"AUDIO",
		Vector2( 160, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressAudio,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 2 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"TEST",
		Vector2( 160, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressDebug,
	},
	{
		Vector2( 320 - 100, 200 + 64 * 3 ),
		Vector2( 200, 48 ),
		optionbackcolor,
		"(BACK)",
		Vector2( 100, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressBack,
	},
};


//
// VIDEO MENU

/*
resolution				[-] <width>x<height> [+]
fullscreen				[off] [on]
vertical sync			[off] [on]
multisample				[-] <samples> [+]
motion blur steps		[-] <steps> [+]
motion blur strength	[-] <blur %> [+]
*/

extern ShellMenuItem shellmenuvideoitems[];

#if defined(USE_SDL)
SDL_Rect **shellmenuvideoresolutions;
SDL_Rect **shellmenuvideoresolution;
#endif
char shellmenuvideoresolutiontext[32];
char shellmenuvideomultisampletext[8];
char shellmenuvideomotionblurstepstext[8];
char shellmenuvideomotionblurtimetext[8];

void ShellMenuVideoEnter()
{
#if defined(USE_SDL)
	shellmenuvideoresolutions = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
	shellmenuvideoresolution = shellmenuvideoresolutions;
	for (SDL_Rect **mode = shellmenuvideoresolutions; *mode != NULL; ++mode)
	{
		if ((*mode)->w <= SCREEN_WIDTH && (*mode)->h <= SCREEN_HEIGHT)
		{
			shellmenuvideoresolution = mode;
			break;
		}
	}
	TIXML_SNPRINTF(shellmenuvideoresolutiontext, sizeof(shellmenuvideoresolutiontext), "%dx%d", (*shellmenuvideoresolution)->w, (*shellmenuvideoresolution)->h);
#endif

	VarItem::CreateInteger("shell.menu.video.fullscreen", SCREEN_FULLSCREEN, 0, 1);
	VarItem::CreateInteger("shell.menu.video.verticalsync", OPENGL_SWAPCONTROL, 0, 1);

	VarItem *varmultisample = VarItem::CreateInteger("shell.menu.video.multisample", OPENGL_MULTISAMPLE, 1, 16);
	TIXML_SNPRINTF(shellmenuvideomultisampletext, sizeof(shellmenuvideomultisampletext), "%dx", varmultisample->GetInteger());

	VarItem *varmotionblursteps = VarItem::CreateInteger("shell.menu.video.motionblur", MOTIONBLUR_STEPS, 1);
	TIXML_SNPRINTF(shellmenuvideomotionblurstepstext, sizeof(shellmenuvideomotionblurstepstext), "%d", varmotionblursteps->GetInteger());

	VarItem *varmotionblurtime = VarItem::CreateInteger("shell.menu.video.motionblurtime", xs_RoundToInt(MOTIONBLUR_TIME * 600), 0, 10);
	TIXML_SNPRINTF(shellmenuvideomotionblurtimetext, sizeof(shellmenuvideomotionblurtimetext), "%d%%", varmotionblurtime->GetInteger() * 10);
}

void ShellMenuVideoExit()
{
#if defined(USE_SDL)
	shellmenuvideoresolutions = NULL;
	shellmenuvideoresolution = NULL;
#endif
}

void ShellMenuVideoPressAccept()
{
#if defined(USE_SDL)
	SCREEN_WIDTH = (*shellmenuvideoresolution)->w;
	SCREEN_HEIGHT = (*shellmenuvideoresolution)->h;
#endif
	SCREEN_FULLSCREEN = VarItem::GetInteger("shell.menu.video.fullscreen") != 0;
	OPENGL_SWAPCONTROL = VarItem::GetInteger("shell.menu.video.verticalsync") != 0;
	OPENGL_MULTISAMPLE = VarItem::GetInteger("shell.menu.video.multisample");
	MOTIONBLUR_STEPS = VarItem::GetInteger("shell.menu.video.motionblur");
	MOTIONBLUR_TIME = VarItem::GetInteger("shell.menu.video.motionblurtime") / 600.0f;

	WritePreferences("preferences.xml");
	InitWindowAction();

	shellmenu.Pop();
}

void ShellMenuVideoPressCancel()
{
	shellmenu.Pop();
}

void ShellMenuVideoPressResolutionUp()
{
#if defined(USE_SDL)
	if (shellmenuvideoresolution > shellmenuvideoresolutions)
	{
		--shellmenuvideoresolution;
		sprintf(shellmenuvideoresolutiontext, "%dx%d", (*shellmenuvideoresolution)->w, (*shellmenuvideoresolution)->h);
	}
#endif
}

void ShellMenuVideoPressResolutionDown()
{
#if defined(USE_SDL)
	if (*(shellmenuvideoresolution+1) != NULL)
	{
		++shellmenuvideoresolution;
		sprintf(shellmenuvideoresolutiontext, "%dx%d", (*shellmenuvideoresolution)->w, (*shellmenuvideoresolution)->h);
	}
#endif
}

void ShellMenuVideoPressFullScreenOff()
{
	VarItem::SetInteger("shell.menu.video.fullscreen", 0);
}

void ShellMenuVideoPressFullScreenOn()
{
	VarItem::SetInteger("shell.menu.video.fullscreen", 1);
}

void ShellMenuVideoPressVerticalSyncOff()
{
	VarItem::SetInteger("shell.menu.video.verticalsync", 0);
}

void ShellMenuVideoPressVerticalSyncOn()
{
	VarItem::SetInteger("shell.menu.video.verticalsync", 1);
}

void ShellMenuVideoPressMultisampleUp()
{
	if (VarItem *item = Database::varitem.Get(0x31bca13e /* "shell.menu.video.multisample" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuvideomultisampletext, "%dx", item->GetInteger());
	}
}

void ShellMenuVideoPressMultisampleDown()
{
	if (VarItem *item = Database::varitem.Get(0x31bca13e /* "shell.menu.video.multisample" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuvideomultisampletext, "%dx", item->GetInteger());
	}
}

void ShellMenuVideoPressMotionBlurStepsUp()
{
	if (VarItem *item = Database::varitem.Get(0x32e32e54 /* "shell.menu.video.motionblur" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuvideomotionblurstepstext, "%d", item->GetInteger());
	}
}

void ShellMenuVideoPressMotionBlurStepsDown()
{
	if (VarItem *item = Database::varitem.Get(0x32e32e54 /* "shell.menu.video.motionblur" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuvideomotionblurstepstext, "%d", item->GetInteger());
	}
}

void ShellMenuVideoPressMotionBlurTimeUp()
{
	if (VarItem *item = Database::varitem.Get(0xfdcc24f5 /* "shell.menu.video.motionblurtime" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuvideomotionblurtimetext, "%d%%", item->GetInteger() * 10);
	}
}

void ShellMenuVideoPressMotionBlurTimeDown()
{
	if (VarItem *item = Database::varitem.Get(0xfdcc24f5 /* "shell.menu.video.motionblurtime" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuvideomotionblurtimetext, "%d%%", item->GetInteger() * 10);
	}
}

ShellMenuItem shellmenuvideoitems[] =
{
	{
		Vector2( 40, 220 - 24 - 16 ),
		Vector2( 560, 12 ),
		optionbackcolor,
		"VIDEO",
		Vector2( 280, 6 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_SELECTED,
		NULL,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 0 ),
		Vector2( 240, 24 ),
		NULL,
		"Resolution",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressResolutionDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 0 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideoresolutiontext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressResolutionUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 1 ),
		Vector2( 240, 24 ),
		NULL,
		"Full Screen",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 1 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"Off",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressFullScreenOff,
		0x9ddbd712 /* "shell.menu.video.fullscreen" */,
		0
	},
	{
		Vector2( 320 + 20 + 110 + 10, 220 + 32 * 1 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"On",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressFullScreenOn,
		0x9ddbd712 /* "shell.menu.video.fullscreen" */,
		1
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 2 ),
		Vector2( 240, 24 ),
		NULL,
		"Vertical Sync",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 2 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"Off",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressVerticalSyncOff,
		0x97eea3ca /* "shell.menu.video.verticalsync" */,
		0
	},
	{
		Vector2( 320 + 20 + 110 + 10, 220 + 32 * 2 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"On",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressVerticalSyncOn,
		0x97eea3ca /* "shell.menu.video.verticalsync" */,
		1
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 3 ),
		Vector2( 240, 24 ),
		NULL,
		"Multisampling",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 3 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMultisampleDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 3 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideomultisampletext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 3 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMultisampleUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 4 ),
		Vector2( 240, 24 ),
		NULL,
		"Motion Steps",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 4 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurStepsDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 4 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideomotionblurstepstext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 4 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurStepsUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 5 ),
		Vector2( 240, 24 ),
		NULL,
		"Motion Blur",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 5 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurTimeDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 5 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideomotionblurtimetext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 5 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurTimeUp,
	},
	{
		Vector2( 40, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"ACCEPT",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressAccept,
	},
	{
		Vector2( 600 - 240, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"CANCEL",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressCancel,
	},
};

//
// AUDIO MENU

/*
channels				[-] <channels> [+]
volume					[-] <volume %> [+]
test					[-] <sound> [+]
*/

char shellmenuaudiosoundchannelstext[8];
char shellmenuaudiosoundvolumetext[8];

void ShellMenuAudioEnter()
{
	VarItem *varsoundchannels = VarItem::CreateInteger("shell.menu.audio.channels", SOUND_CHANNELS, 1);
	TIXML_SNPRINTF(shellmenuaudiosoundchannelstext, sizeof(shellmenuaudiosoundchannelstext), "%d", varsoundchannels->GetInteger());

	VarItem *varsoundvolume = VarItem::CreateInteger("shell.menu.audio.volume", xs_RoundToInt(SOUND_VOLUME * 10), 0, 20);
	TIXML_SNPRINTF(shellmenuaudiosoundvolumetext, sizeof(shellmenuaudiosoundvolumetext), "%d%%", varsoundvolume->GetInteger() * 10);
}

void ShellMenuAudioExit()
{
}

void ShellMenuAudioPressAccept()
{
	SOUND_CHANNELS = VarItem::GetInteger("shell.menu.audio.channels");
	SOUND_VOLUME = VarItem::GetInteger("shell.menu.audio.volume") / 10.0f;

	WritePreferences("preferences.xml");

	shellmenu.Pop();
}

void ShellMenuAudioPressCancel()
{
	shellmenu.Pop();
}


void ShellMenuAudioPressSoundChannelsUp()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
	}
}

void ShellMenuAudioPressSoundChannelsDown()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
	}
}

void ShellMenuAudioPressSoundVolumeUp()
{
	if (VarItem *item = Database::varitem.Get(0xf97c9992 /* "shell.menu.audio.volume" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundvolumetext, "%d%%", item->GetInteger() * 10);
	}
}

void ShellMenuAudioPressSoundVolumeDown()
{
	if (VarItem *item = Database::varitem.Get(0xf97c9992 /* "shell.menu.audio.volume" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundvolumetext, "%d%%", item->GetInteger() * 10);
	}
}


ShellMenuItem shellmenuaudioitems[] = 
{
	{
		Vector2( 40, 220 - 24 - 16 ),
		Vector2( 560, 12 ),
		optionbackcolor,
		"AUDIO",
		Vector2( 280, 6 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_SELECTED,
		NULL,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 0 ),
		Vector2( 240, 24 ),
		NULL,
		"Mixer Channels",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundChannelsDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 0 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundchannelstext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundChannelsUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 1 ),
		Vector2( 240, 24 ),
		NULL,
		"Mixer Volume",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 1 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 1 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundvolumetext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 1 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeUp,
	},
	{
		Vector2( 40, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"ACCEPT",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressAccept,
	},
	{
		Vector2( 600 - 240, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"CANCEL",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressCancel,
	},
};

//
// TEST MENU

/*
simulation rate			[-] <rate> [+]
time scale				[-] <scale %> [+]
profile screen			[off] [on]
profile print			[off] [on]
framerate screen		[off] [on]
framerate print			[off] [on]
*/

//
// SHELL

ShellMenuPage shellmenumainpage =
{
	shellmenumainitems, SDL_arraysize(shellmenumainitems), NULL, NULL
};
ShellMenuPage shellmenuoptionspage =
{
	shellmenuoptionsitems, SDL_arraysize(shellmenuoptionsitems)
};
ShellMenuPage shellmenuvideopage =
{
	shellmenuvideoitems, SDL_arraysize(shellmenuvideoitems), ShellMenuVideoEnter, ShellMenuVideoExit
};
ShellMenuPage shellmenuaudiopage =
{
	shellmenuaudioitems, SDL_arraysize(shellmenuaudioitems), ShellMenuAudioEnter, ShellMenuAudioExit
};

ShellMenu shellmenu =
{
	0
};

// draw options
void RenderOptions(ShellMenu &menu, unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// cursor position
	float cursor_x = 320 - 240 * input.value[Input::AIM_HORIZONTAL];
	float cursor_y = 240 - 240 * input.value[Input::AIM_VERTICAL];

	// HACK use the main page
	ShellMenuPage &page = *menu.mActive;

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
				// mark as rollover
				option.mState |= BUTTON_ROLLOVER;

				// if mouse button pressed...
				if (input.value[Input::FIRE_PRIMARY])
				{
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

				if (!input.value[Input::FIRE_PRIMARY])
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

// render shell options
void RenderShellOptions(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	RenderOptions(shellmenu, aId, aTime, aTransform);
}


// enter shell state
void EnterShellState()
{
	// clear the screen
	glClear(
		GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_TEST
		| GL_DEPTH_BUFFER_BIT
#endif
		);

#if defined(USE_SDL)
	// show the screen
	SDL_GL_SwapBuffers();
#elif defined(USE_SFML)
	window.Display();
#elif defined(USE_GLFW)
	glfwSwapBuffers();
#endif

	// reset simulation timer
	sim_rate = float(SIMULATION_RATE);
	sim_step = 1.0f / sim_rate;
	sim_turn = 0;
	sim_fraction = 1.0f;

	// input binding
	InitInput(INPUT_CONFIG.c_str());

	// level configuration
	InitLevel("shell.xml");

	// start audio
	Sound::Resume();

	// create title overlay
	ShellTitle *title = new ShellTitle(0x9865b509 /* "title" */);
	Database::overlay.Put(0x9865b509 /* "title" */, title);
	title->Show();

	// create options overlay
	shellmenu.mActive = NULL;
	shellmenu.Push(&shellmenumainpage);
	Overlay *options = new Overlay(0xef286ca5 /* "options" */);
	Database::overlay.Put(0xef286ca5 /* "options" */, options);
	options->SetAction(Overlay::Action(RenderShellOptions));
	options->Show();

	// set to runtime mode
	runtime = true;
}

void ExitShellState()
{
	// stop audio
	Sound::Pause();

	// stop any startup sound (HACK)
	StopSoundCue(0x94326baa /* "startup" */);

	// clear overlays
	delete Database::overlay.Get(0x9865b509 /* "title" */);
	Database::overlay.Delete(0x9865b509 /* "title" */);
	delete Database::overlay.Get(0xef286ca5 /* "options" */);
	Database::overlay.Delete(0xef286ca5 /* "options" */);

	// clear all databases
	Database::Cleanup();

	// collidable done
	Collidable::WorldDone();

	// set to non-runtime mode
	runtime = false;
}


//
// ESCAPE MENU

extern void EscapeMenuExit();

void EscapeMainMenuPressContinue(void)
{
	EscapeMenuExit();
}

void EscapeMainMenuPressRestart(void)
{
	setgamestate = STATE_RELOAD;
	EscapeMenuExit();
}

void EscapeMainMenuPressMain(void)
{
	setgamestate = STATE_SHELL;
	EscapeMenuExit();
}

ShellMenuItem escapemenumainitems[] =
{
	{
		Vector2( 320 - 160, 200 + 64 * 0 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"CONTINUE",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		EscapeMainMenuPressContinue,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 1 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"RESTART",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		EscapeMainMenuPressRestart,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 2 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"OPTIONS",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressOptions,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 3 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"MAIN",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		EscapeMainMenuPressMain,
	},
};

ShellMenuPage escapemenumainpage =
{
	escapemenumainitems, SDL_arraysize(escapemenumainitems), NULL, NULL
};

// render shell options
void RenderEscapeOptions(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// darken the screen
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(640, 0);
	glVertex2f(640, 480);
	glVertex2f(0, 480);
	glEnd();

	// render options
	RenderOptions(shellmenu, aId, aTime, aTransform);
}


// enter escape menu
void EscapeMenuEnter(void)
{
	escape = true;
	if (Overlay *overlay = Database::overlay.Get(0x9e212406 /* "escape" */))
	{
		for (Database::Typed<PlayerHUD *>::Iterator itor(&Database::playerhud); itor.IsValid(); ++itor)
			itor.GetValue()->Hide();
		shellmenu.mActive = NULL;
		shellmenu.Push(&escapemenumainpage);
		overlay->Show();
	}
}

// exit escape menu
void EscapeMenuExit(void)
{
	if (Overlay *overlay = Database::overlay.Get(0x9e212406 /* "escape" */))
	{
		for (Database::Typed<PlayerHUD *>::Iterator itor(&Database::playerhud); itor.IsValid(); ++itor)
			itor.GetValue()->Show();
		overlay->Hide();
	}
	escape = false;
}
