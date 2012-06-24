#include "StdAfx.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"


extern ShellMenuPage shellmenuvideopage;
extern ShellMenuPage shellmenuaudiopage;

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
ShellMenuPage shellmenuoptionspage =
{
	shellmenuoptionsitems, SDL_arraysize(shellmenuoptionsitems)
};
