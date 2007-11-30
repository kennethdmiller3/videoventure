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

// configure
bool Renderable::Configure(TiXmlElement *element)
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
					DrawListMap::iterator itor = drawlists.find(Hash(name));
					if (itor != drawlists.end())
					{
						// use the named drawlist
						mDraw = itor->second;
						break;
					}
				}

				if (child->FirstChildElement())
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
					break;
				}
			}
			break;

		default:
			break;
		}
	}

	return true;
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
