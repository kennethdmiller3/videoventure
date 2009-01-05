#include "StdAfx.h"

namespace Platform
{
	extern void PrintAttributes(void);

	bool Init(void)
	{
		// initialize SDL
		if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
			return false;    

		// Check for joystick
		if (SDL_NumJoysticks() > 0)
		{
			// Open joystick
			SDL_Joystick *joy = SDL_JoystickOpen(0);
			if(joy)
			{
				DebugPrint("Opened Joystick 0\n");
				DebugPrint("Name: %s\n", SDL_JoystickName(0));
			}
		}

		// create the window
		OpenWindow();

		// set window title
		SDL_WM_SetCaption( "Shmup!", NULL );

		// platform-specific attributes
		PrintAttributes();

		// success!
		return true;    
	}

	void Done(void)
	{
		// quit SDL
		SDL_Quit();
	}
}
