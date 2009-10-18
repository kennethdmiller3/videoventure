#include "StdAfx.h"

#include "Shell.h"
#include "GameState.h"
#include "VarItem.h"
#include "oglconsole.h"
#include "Title.h"
#include "Sound.h"
#include "Collidable.h"
#include "PlayerHUD.h"
#include "Preferences.h"
#include "Library.h"


// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_CreateFont();
extern "C" void OGLCONSOLE_Resize(OGLCONSOLE_Console console);

extern bool escape;
extern bool paused;

extern void UpdateWindowAction(void);

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

// color typedef (HACK)
typedef Color4 Color4_2[2];

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

struct Size
{
	unsigned short w;
	unsigned short h;
};

Size shellmenuvideoresolutionlist[256];
int shellmenuvideoresolutioncount;
int shellmenuvideoresolutionindex;
char shellmenuvideoresolutiontext[32];
char shellmenuvideomultisampletext[8];
char shellmenuvideomotionblurstepstext[8];
char shellmenuvideomotionblurtimetext[8];

static void ShellMenuVideoUpdateResolutionText(void)
{
	TIXML_SNPRINTF(
		shellmenuvideoresolutiontext,
		sizeof(shellmenuvideoresolutiontext),
		"%dx%d",
		shellmenuvideoresolutionlist[shellmenuvideoresolutionindex].w,
		shellmenuvideoresolutionlist[shellmenuvideoresolutionindex].h
		);
}

static void ShellMenuVideoUpdateResolutionList(void)
{
	shellmenuvideoresolutioncount = 0;
#if defined(USE_SDL)
	// SDL returns modes sorted by decreasing resolution (!)
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
	int modecount = 0;
	for (SDL_Rect **mode = modes; *mode != NULL; ++mode)
		++modecount;
	for (int i = 0; i < modecount; ++i)
	{
		shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].w = modes[modecount-1-i]->w;
		shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].h = modes[modecount-1-i]->h;
		++shellmenuvideoresolutioncount;
	}
#elif defined(USE_SFML)
	int modecount = sf::VideoMode::GetModesCount();
	int depth = SCREEN_DEPTH ? SCREEN_DEPTH : 32
	for (int i = 0; i < modecount; ++i)
	{
		sf::VideoMode mode(sf::VideoMode::GetMode(i));
		if (mode.BitsPerPixel == depth)
		{
			shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].w = modes[i].Width;
			shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].h = modes[i].Height;
			++shellmenuvideoresolutioncount;
		}
	}
#elif defined(USE_GLFW)
	// GLFW returns modes sorted by increasing depth, then by increasing resolution
	GLFWvidmode *modes = static_cast<GLFWvidmode *>(_alloca(256 * sizeof(GLFWvidmode)));
	int modecount = glfwGetVideoModes(modes, 256);
	int depth = SCREEN_DEPTH ? std::min(SCREEN_DEPTH, 24) : 24;		// HACK
	for (int i = 0; i < modecount; ++i)
	{
		if (modes[i].RedBits + modes[i].GreenBits + modes[i].BlueBits == depth)
		{
			shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].w = static_cast<unsigned short>(modes[i].Width);
			shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].h = static_cast<unsigned short>(modes[i].Height);
			++shellmenuvideoresolutioncount;
		}
	}
#endif
}

void ShellMenuVideoEnter()
{
	ShellMenuVideoUpdateResolutionList();

	shellmenuvideoresolutionindex = 0;
	for (int i = 0; i < shellmenuvideoresolutioncount; ++i)
	{
		if (SCREEN_WIDTH >= shellmenuvideoresolutionlist[i].w &&
			SCREEN_HEIGHT >= shellmenuvideoresolutionlist[i].h)
			shellmenuvideoresolutionindex = i;
	}
	ShellMenuVideoUpdateResolutionText();

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
}

void ShellMenuVideoPressAccept()
{
	SCREEN_WIDTH = shellmenuvideoresolutionlist[shellmenuvideoresolutionindex].w;
	SCREEN_HEIGHT = shellmenuvideoresolutionlist[shellmenuvideoresolutionindex].h;
	SCREEN_FULLSCREEN = VarItem::GetInteger("shell.menu.video.fullscreen") != 0;
	OPENGL_SWAPCONTROL = VarItem::GetInteger("shell.menu.video.verticalsync") != 0;
	OPENGL_MULTISAMPLE = VarItem::GetInteger("shell.menu.video.multisample");
	MOTIONBLUR_STEPS = VarItem::GetInteger("shell.menu.video.motionblur");
	MOTIONBLUR_TIME = VarItem::GetInteger("shell.menu.video.motionblurtime") / 600.0f;

	WritePreferences("preferences.xml");
	UpdateWindowAction();

	shellmenu.Pop();
}

void ShellMenuVideoPressCancel()
{
	shellmenu.Pop();
}

void ShellMenuVideoPressResolutionDown()
{
	if (shellmenuvideoresolutionindex > 0)
	{
		--shellmenuvideoresolutionindex;
		ShellMenuVideoUpdateResolutionText();
	}
}

void ShellMenuVideoPressResolutionUp()
{
	if (shellmenuvideoresolutionindex < shellmenuvideoresolutioncount - 1)
	{
		++shellmenuvideoresolutionindex;
		ShellMenuVideoUpdateResolutionText();
	}
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
effects volume			[-] <volume %> [+]
music volume			[-] <volume %> [+]
test					[-] <sound> [+]
*/

int shellmenuaudiosoundchannelsonenter;
char shellmenuaudiosoundchannelstext[8];
float shellmenuaudiosoundvolumeeffectonenter;
char shellmenuaudiosoundvolumeeffecttext[8];
float shellmenuaudiosoundvolumemusiconenter;
char shellmenuaudiosoundvolumemusictext[8];

void ShellMenuAudioEnter()
{
	shellmenuaudiosoundchannelsonenter = SOUND_CHANNELS;
	VarItem *varsoundchannels = VarItem::CreateInteger("shell.menu.audio.channels", SOUND_CHANNELS, 1);
	TIXML_SNPRINTF(shellmenuaudiosoundchannelstext, sizeof(shellmenuaudiosoundchannelstext), "%d", varsoundchannels->GetInteger());

	shellmenuaudiosoundvolumeeffectonenter = SOUND_VOLUME_EFFECT;
	VarItem *varsoundvolumeeffect = VarItem::CreateInteger("shell.menu.audio.volume.effect", xs_RoundToInt(SOUND_VOLUME_EFFECT * 10), 0, 20);
	TIXML_SNPRINTF(shellmenuaudiosoundvolumeeffecttext, sizeof(shellmenuaudiosoundvolumeeffecttext), "%d%%", varsoundvolumeeffect->GetInteger() * 10);

	shellmenuaudiosoundvolumemusiconenter = SOUND_VOLUME_MUSIC;
	VarItem *varsoundvolumemusic = VarItem::CreateInteger("shell.menu.audio.volume.music", xs_RoundToInt(SOUND_VOLUME_MUSIC * 10), 0, 20);
	TIXML_SNPRINTF(shellmenuaudiosoundvolumemusictext, sizeof(shellmenuaudiosoundvolumemusictext), "%d%%", varsoundvolumemusic->GetInteger() * 10);
}

void ShellMenuAudioExit()
{
}

void ShellMenuAudioPressAccept()
{
	SOUND_CHANNELS = VarItem::GetInteger("shell.menu.audio.channels");
	SOUND_VOLUME_EFFECT = VarItem::GetInteger("shell.menu.audio.volume.effect") / 10.0f;
	SOUND_VOLUME_MUSIC = VarItem::GetInteger("shell.menu.audio.volume.music") / 10.0f;

	WritePreferences("preferences.xml");

	UpdateSoundVolume();

	shellmenu.Pop();
}

void ShellMenuAudioPressCancel()
{
	SOUND_CHANNELS = shellmenuaudiosoundchannelsonenter;
	SOUND_VOLUME_EFFECT = shellmenuaudiosoundvolumeeffectonenter;
	SOUND_VOLUME_MUSIC = shellmenuaudiosoundvolumemusiconenter;

	UpdateSoundVolume();

	shellmenu.Pop();
}


void ShellMenuAudioPressSoundChannelsUp()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
		SOUND_CHANNELS = item->GetInteger();
	}
}

void ShellMenuAudioPressSoundChannelsDown()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
		SOUND_CHANNELS = item->GetInteger();
	}
}

void ShellMenuAudioPressSoundVolumeEffectUp()
{
	if (VarItem *item = Database::varitem.Get(0x686112dd /* "shell.menu.audio.volume.effect" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundvolumeeffecttext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_EFFECT = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
	}
}

void ShellMenuAudioPressSoundVolumeEffectDown()
{
	if (VarItem *item = Database::varitem.Get(0x686112dd /* "shell.menu.audio.volume.effect" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundvolumeeffecttext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_EFFECT = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
	}
}

void ShellMenuAudioPressSoundVolumeMusicUp()
{
	if (VarItem *item = Database::varitem.Get(0xea502ecf /* "shell.menu.audio.volume.music" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundvolumemusictext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_MUSIC = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
	}
}

void ShellMenuAudioPressSoundVolumeMusicDown()
{
	if (VarItem *item = Database::varitem.Get(0xea502ecf /* "shell.menu.audio.volume.music" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundvolumemusictext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_MUSIC = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
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
		"Effects Volume",
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
		ShellMenuAudioPressSoundVolumeEffectDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 1 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundvolumeeffecttext,
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
		ShellMenuAudioPressSoundVolumeEffectUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 2 ),
		Vector2( 240, 24 ),
		NULL,
		"Music Volume",
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
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeMusicDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 2 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundvolumemusictext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 2 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeMusicUp,
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
	float cursor_x = 320 - 240 * input.value[Input::MENU_HORIZONTAL];
	float cursor_y = 240 - 240 * input.value[Input::MENU_VERTICAL];

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

	// show back buffer
	Platform::Present();

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

	// free any loaded libraries
	FreeLibraries();

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
	if (!paused)
		Sound::Pause();
}

// exit escape menu
void EscapeMenuExit(void)
{
	if (!paused)
		Sound::Resume();
	if (Overlay *overlay = Database::overlay.Get(0x9e212406 /* "escape" */))
	{
		for (Database::Typed<PlayerHUD *>::Iterator itor(&Database::playerhud); itor.IsValid(); ++itor)
			itor.GetValue()->Show();
		overlay->Hide();
	}
	escape = false;
}
