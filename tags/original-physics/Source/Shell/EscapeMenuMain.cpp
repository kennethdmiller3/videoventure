#include "StdAfx.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "GameState.h"


extern ShellMenuPage shellmenuoptionspage;

//
// ESCAPE MENU

extern void EscapeMenuExit();

void EscapeMenuMainPressContinue(void)
{
	EscapeMenuExit();
}

void EscapeMenuMainPressRestart(void)
{
	setgamestate = STATE_RELOAD;
	EscapeMenuExit();
}

void EscapeMenuMainPressOptions(void)
{
	shellmenu.Push(&shellmenuoptionspage);
}

void EscapeMenuMainPressMain(void)
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
		EscapeMenuMainPressContinue,
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
		EscapeMenuMainPressRestart,
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
		EscapeMenuMainPressOptions,
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
		EscapeMenuMainPressMain,
	},
};

ShellMenuPage escapemenumainpage =
{
	escapemenumainitems, SDL_arraysize(escapemenumainitems), NULL, NULL
};
