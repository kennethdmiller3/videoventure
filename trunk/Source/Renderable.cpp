#include "StdAfx.h"
#include "Renderable.h"

namespace Database
{
	Typed<RenderableTemplate> renderabletemplate("renderabletemplate");
	Typed<Renderable> renderable("renderable");
	Typed<GLuint> drawlist("drawlist");
}

RenderableTemplate::RenderableTemplate(void)
: mDraw(0)
{
}

RenderableTemplate::~RenderableTemplate(void)
{
}

// configure
bool RenderableTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x109dd1ad /* "renderable" */)
		return false;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xc98b019b /* "drawlist" */:
			{
				// get the list name
				const char *name = child->Attribute("name");
				if (name)
				{
					// find the named drawlist
					// (or 0 if not found)
					mDraw = Database::drawlist.Get(Hash(name));
				}
				else if (child->FirstChildElement())
				{
					// create a new draw list
					GLuint handle = glGenLists(1);
					glNewList(handle, GL_COMPILE);

					// process draw items
					ProcessDrawItems(child);

					// finish the draw list
					glEndList();

					// use the anonymous drawlist
					mDraw = handle;
				}
			}
			break;

		default:
			break;
		}
	}

	return true;
}



Renderable::List Renderable::sAll;

Renderable::Renderable(void)
: RenderableTemplate(), show(false)
{
}

Renderable::Renderable(const RenderableTemplate &aTemplate)
: RenderableTemplate(aTemplate), show(false)
{
	Show();
}

Renderable::~Renderable(void)
{
	Hide();
}

void Renderable::Show(void)
{
	Hide();

	if (!show)
	{
#ifdef DRAW_FRONT_TO_BACK
		entry = sAll.insert(sAll.begin(), this);
#else
		entry = sAll.insert(sAll.end(), this);
#endif
		show = true;
	}
}

void Renderable::Hide(void)
{
	if (show)
	{
		sAll.erase(entry);
		entry = sAll.end();
		show = false;
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

void Renderable::Render(void)
{
	glCallList(mDraw);
};
