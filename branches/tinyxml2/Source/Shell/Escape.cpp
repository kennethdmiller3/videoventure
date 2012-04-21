#include "StdAfx.h"

#include "Escape.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "Overlay.h"
#include "PlayerHUD.h"
#include "Sound.h"


extern bool escape;
extern bool paused;

extern ShellMenuPage escapemenumainpage;


//
// ESCAPE

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
	shellmenu.RenderOptions(aId, aTime, aTransform);
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