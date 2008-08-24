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


Overlay *Overlay::sHead;
Overlay *Overlay::sTail;

Overlay::Overlay(unsigned int aId)
: mId(aId)
, mNext(NULL)
, mPrev(NULL)
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
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
		mActive = true;
	}
}

void Overlay::Hide(void)
{
	if (mActive)
	{
		if (sHead == this)
			sHead = mNext;
		if (sTail == this)
			sTail = mPrev;
		if (mNext)
			mNext->mPrev = mPrev;
		if (mPrev)
			mPrev->mNext = mNext;
		mNext = NULL;
		mPrev = NULL;
		mActive = false;
	}
}

void Overlay::RenderAll()
{
	// render all overlays
	Overlay *itor = sHead;
	while (itor)
	{
		// get the next iterator
		// (in case the entry gets deleted)
		Overlay *next = itor->mNext;

		// get the overlay template
		const OverlayTemplate &overlay = Database::overlaytemplate.Get(itor->mId);

		// elapsed time
		float t = fmodf((int(sim_turn - itor->mStart) + sim_fraction - itor->mFraction) * sim_step, overlay.mPeriod);

		// perform action
		(itor->mAction)(itor->mId, t, 0.0f, 0.0f, 0.0f);

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
				ProcessDrawItems(element, buffer);
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
