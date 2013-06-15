#include "StdAfx.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "VarItem.h"
#include "Preferences.h"


extern void UpdateWindowAction(void);

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
	// get the primary monitor
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();

	// GLFW returns modes sorted by increasing depth, then by increasing resolution
	int modecount;
	const GLFWvidmode* modes = glfwGetVideoModes(monitor, &modecount);

	int depth = SCREEN_DEPTH ? std::min(SCREEN_DEPTH, 24) : 24;		// HACK
	for (int i = 0; i < modecount; ++i)
	{
		if (modes[i].redBits + modes[i].greenBits + modes[i].blueBits == depth)
		{
			shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].w = static_cast<unsigned short>(modes[i].width);
			shellmenuvideoresolutionlist[shellmenuvideoresolutioncount].h = static_cast<unsigned short>(modes[i].height);
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
ShellMenuPage shellmenuvideopage =
{
	shellmenuvideoitems, SDL_arraysize(shellmenuvideoitems), ShellMenuVideoEnter, ShellMenuVideoExit
};
