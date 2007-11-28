#include "StdAfx.h"
#include "Renderable.h"

Renderable::List Renderable::sAll;

Renderable::Renderable(void)
: mDraw(0)
{
	entry = sAll.end();
	Show();
}

Renderable::~Renderable(void)
{
	Hide();
}

void Renderable::Show(void)
{
	Hide();

#ifdef DRAW_FRONT_TO_BACK
	entry = sAll.insert(sAll.begin(), this);
#else
	entry = sAll.insert(sAll.end(), this);
#endif
}

void Renderable::Hide(void)
{
	if (entry != sAll.end())
	{
		sAll.erase(entry);
		entry = sAll.end();
	}
}

void Renderable::RenderAll(void)
{
	// render all renderables
	List::iterator itor = sAll.begin();
	while (itor != sAll.end())
	{
		// get the next iterator
		// (in case the entry gets deleted)
		List::iterator next(itor);
		++next;

		// render
		(*itor)->Render();

		// go to the next iterator
		itor = next;
	}
}
