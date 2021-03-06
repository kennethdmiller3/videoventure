#include "StdAfx.h"

#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "GameState.h"
#include "VarItem.h"
#include "Overlay.h"
#include "Sound.h"
#include "Collidable.h"
#include "Library.h"
#include "Font.h"
#include "Drawlist.h"
#include "Texture.h"


extern bool InitInput(const char *config);
extern bool InitLevel(const char *config);


// forward declaration
extern ShellMenuPage shellmenumainpage;


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

ShellMenu shellmenu =
{
	0
};


// render shell options
void RenderShellOptions(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	shellmenu.RenderOptions(aId, aTime, aTransform);
}


// enter shell state
void EnterShellState()
{
	// set up drawlists
	InitDrawlists();

	// create default font
	CreateDefaultFont();

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

	// create options overlay
	shellmenu.mActive = NULL;
	shellmenu.Push(&shellmenumainpage);
	Overlay *options = new Overlay(0xef286ca5 /* "options" */);
	Database::overlay.Put(0xef286ca5 /* "options" */, options);
	options->SetAction(Overlay::Action(RenderShellOptions));
	options->Show();

	// start audio
	Sound::Resume();

	// play the startup sound (HACK)
	PlaySoundCue(0x94326baa /* "startup" */);

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
	delete Database::overlay.Get(0xef286ca5 /* "options" */);
	Database::overlay.Delete(0xef286ca5 /* "options" */);

	// cleanup drawlists
	CleanupDrawlists();

	// cleanup textures
	CleanupTextures();

	// clear all databases
	Database::Cleanup();

	// free any loaded libraries
	FreeLibraries();

	// collidable done
	Collidable::WorldDone();

	// set to non-runtime mode
	runtime = false;
}
