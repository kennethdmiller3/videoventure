#include "StdAfx.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "GameState.h"


extern ShellMenuPage shellmenuoptionspage;

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
ShellMenuPage shellmenumainpage =
{
	shellmenumainitems, SDL_arraysize(shellmenumainitems), NULL, NULL
};
