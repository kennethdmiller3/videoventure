#include "StdAfx.h"
#include "Overlay.h"
#include "Drawlist.h"
#include "Entity.h"


OverlayTemplate::OverlayTemplate(void)
: mPeriod(FLT_MAX)
{
}

OverlayTemplate::~OverlayTemplate(void)
{
}

// configure
bool OverlayTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	if (Hash(element->Value()) != 0x109dd1ad /* "overlay" */)
		return false;

	// animation period
	element->QueryFloatAttribute("period", &mPeriod);

	return true;
}


Overlay Overlay::sRoot(0);

Overlay::Overlay(unsigned int aId)
: mId(aId)
, mActive(false)
, mAction()
, mStart(sim_turn)
, mFraction(sim_fraction)
{
}

Overlay::~Overlay(void)
{
	Hide();
}

void Overlay::Show(void)
{
	if (!mActive)
	{
		AttachLast(&sRoot);
		mActive = true;
	}
}

void Overlay::Hide(void)
{
	if (mActive)
	{
		mActive = false;
		Detach();
	}
}

void Overlay::RenderAll(Overlay &aRoot)
{
	// render all overlays
	Overlay *itor = aRoot.mFirstChild;
	while (itor)
	{
		// get the next iterator
		// (in case the entry gets deleted)
		Overlay *next = itor->mNextSibling;

		// if the overlay renders...
		if (itor->mAction)
		{
			// get the overlay template
			const OverlayTemplate &overlay = Database::overlaytemplate.Get(itor->mId);

			// elapsed time
			// TO DO: replace this with expressions
			float t = fmodf((int(sim_turn - itor->mStart) + sim_fraction - itor->mFraction) * sim_step, overlay.mPeriod);

			// push a transform
			glPushMatrix();

			// perform action
			// TO DO: support transform parameter
			(itor->mAction)(itor->mId, t, Transform2::Identity());

			// reset the transform
			glPopMatrix();
		}

		// recurse on children
		if (itor->mFirstChild)
			RenderAll(*itor);

		// go to the next iterator
		itor = next;
	}

#ifdef RENDER_STATS
	DebugPrint("d=%d/%d\n", drawn, drawn+culled);
#endif
}


namespace Database
{
	Typed<OverlayTemplate> overlaytemplate(0x2a4c42d1 /* "overlaytemplate" */);
	Typed<Overlay *> overlay(0x2065d503 /* "overlay" */);

	namespace Loader
	{
		class OverlayLoader
		{
		public:
			OverlayLoader()
			{
				AddConfigure(0x2065d503 /* "overlay" */, Entry(this, &OverlayLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				OverlayTemplate &overlay = Database::overlaytemplate.Open(aId);
				overlay.Configure(element, aId);
				Database::overlaytemplate.Close(aId);

				// process child elements
				std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Open(aId);
				ConfigureDrawItems(element, buffer);
				Database::dynamicdrawlist.Close(aId);
			}
		}
		overlayloader;
	}

	namespace Initializer
	{
		class OverlayInitializer
		{
		public:
			OverlayInitializer()
			{
				AddActivate(0x2a4c42d1 /* "overlaytemplate" */, Entry(this, &OverlayInitializer::Activate));
				AddDeactivate(0x2a4c42d1 /* "overlaytemplate" */, Entry(this, &OverlayInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				Overlay *overlay = new Overlay(aId);
				Database::overlay.Put(aId, overlay);
				overlay->SetAction(RenderDrawlist);
				overlay->Show();
			}

			void Deactivate(unsigned int aId)
			{
				if (Overlay *overlay = Database::overlay.Get(aId))
				{
					overlay->Hide();
					delete overlay;
					Database::overlay.Delete(aId);
				}
			}
		}
		overlayinitializer;
	}
}
