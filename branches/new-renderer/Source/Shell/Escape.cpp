#include "StdAfx.h"

#include "Escape.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "Overlay.h"
#include "PlayerHUD.h"
#include "Sound.h"
#include "Render.h"


extern bool escape;
extern bool paused;

extern ShellMenuPage escapemenumainpage;

struct Vertex
{
	Vector3 pos;
#ifdef ESCAPE_FLOAT_COLORS
	Color4 color;
#else
	unsigned int color;
#endif
};

//
// ESCAPE

// render shell options
void RenderEscapeOptions(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// darken the screen
#ifdef ESCAPE_FLOAT_COLORS
	const Color4 color(0.0f, 0.0f, 0.0f, 0.5f);
#else
	const unsigned int color = 0x7F000000;
#endif
	UseProgram(0);
	SetAttribFormat(0, 3, GL_FLOAT);
	SetAttribFormat(2, 4, GL_UNSIGNED_BYTE);
	SetWorkFormat((1<<0)|(1<<2));
	SetDrawMode(GL_TRIANGLES);
	int base = GetVertexCount();
	register Vertex * __restrict v = static_cast<Vertex *>(AllocVertices(4));
	v->pos = Vector3(0.0f, 0.0f, 0.0f);
	v->color = color;
	++v;
	v->pos = Vector3(640.0f, 0.0f, 0.0f);
	v->color = color;
	++v;
	v->pos = Vector3(640.0f, 480.0f, 0.0f);
	v->color = color;
	++v;
	v->pos = Vector3(0.0f, 480.0f, 0.0f);
	v->color = color;
	++v;
	IndexQuads(base, GetVertexCount() - base);

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