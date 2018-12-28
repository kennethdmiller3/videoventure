#include "StdAfx.h"
#include "Renderable.h"
#include "Drawlist.h"
#include "Entity.h"

#ifdef USE_POOL_ALLOCATOR
// renderable pool
static MemoryPool sPool(sizeof(Renderable));
void *Renderable::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Renderable::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<RenderableTemplate> renderabletemplate(0x0cb54133 /* "renderabletemplate" */);
	Typed<Renderable *> renderable(0x109dd1ad /* "renderable" */);

	namespace Loader
	{
		static void RenderableConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			RenderableTemplate &renderable = Database::renderabletemplate.Open(aId);
			renderable.Configure(element, aId);
			Database::renderabletemplate.Close(aId);

			bool inherit = true;
			element->QueryBoolAttribute("inherit", &inherit);

			// process child elements
			std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Open(aId);

			size_t buffersize = buffer.size();

#ifdef DRAW_FRONT_TO_BACK
			if (!inherit)
				buffer.clear();
			ConfigureDrawItems(aId, element, buffer, false);
#else
			std::vector<unsigned int> original;
			std::swap(buffer, original);
			ConfigureDrawItems(aId, element, buffer, false);
			if (inherit)
				buffer.insert(buffer.end(), original.begin(), original.end());
#endif

			DebugPrint("renderable buffer size %d -> %d\n", buffersize, buffer.size());

			Database::dynamicdrawlist.Close(aId);
		}
		Configure renderableconfigure(0x109dd1ad /* "renderable" */, RenderableConfigure);
	}

	namespace Initializer
	{
		static void RenderableActivate(unsigned int aId)
		{
			const RenderableTemplate &renderabletemplate = Database::renderabletemplate.Get(aId);
			Renderable *renderable = new Renderable(renderabletemplate, aId);
			Database::renderable.Put(aId, renderable);
			renderable->SetAction(RenderDynamicDrawlist);
			renderable->Show();
		}
		Activate renderableactivate(0x0cb54133 /* "renderabletemplate" */, RenderableActivate);

		static void RenderableDeactivate(unsigned int aId)
		{
			if (Renderable *renderable = Database::renderable.Get(aId))
			{
				renderable->Hide();
				delete renderable;
				Database::renderable.Delete(aId);
			}
		}
		Deactivate renderabledeactivate(0x0cb54133 /* "renderabletemplate" */, RenderableDeactivate);
	}
}


RenderableTemplate::RenderableTemplate(void)
: mRadius(0)
, mDepth(0)
, mPeriod(FLT_MAX)
, mTransform(true)
{
}

RenderableTemplate::~RenderableTemplate(void)
{
}

// configure
bool RenderableTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	// animation period
	element->QueryFloatAttribute("period", &mPeriod);

	// bounding radius
//	element->QueryFloatAttribute("x", &mBounds.p.x);
//	element->QueryFloatAttribute("y", &mBounds.p.y);
	element->QueryFloatAttribute("radius", &mRadius);

	// sorting depth
	element->QueryFloatAttribute("depth", &mDepth);

	return true;
}


Renderable *Renderable::sHead;
Renderable *Renderable::sTail;

Renderable::Renderable(void)
: mId(0)
, mNext(NULL)
, mPrev(NULL)
, mActive(false)
, mAction()
, mRadius(0)
, mDepth(0)
, mStart(sim_turn)
, mFraction(sim_fraction)
{
}

Renderable::Renderable(const RenderableTemplate &aTemplate, unsigned int aId)
: mId(aId)
, mNext(NULL)
, mPrev(NULL)
, mActive(false)
, mAction()
, mRadius(aTemplate.mRadius)
, mDepth(aTemplate.mDepth)
, mStart(sim_turn)
, mFraction(sim_fraction)
{
}

Renderable::~Renderable(void)
{
	Hide();
}

void Renderable::Show(void)
{
	if (!mActive)
	{
		Renderable *aParent;

#ifdef DRAW_FRONT_TO_BACK
		// TO DO: make this go faster
		for (aParent = sHead; aParent != NULL; aParent = aParent->mNext)
			if (mDepth <= aParent->mDepth)
				break;

		if (aParent)
		{
			mNext = aParent;
			mPrev = aParent->mPrev;
		}
		else
		{
			mNext = NULL;
			mPrev = sTail;
		}
#else
		// TO DO: make this go faster
		for (aParent = sTail; aParent != NULL; aParent = aParent->mPrev)
			if (mDepth <= aParent->mDepth)
				break;

		if (aParent)
		{
			mNext = aParent->mNext;
			mPrev = aParent;
		}
		else
		{
			mNext = sHead;
			mPrev = NULL;
		}
#endif

		if (mNext)
			mNext->mPrev = this;
		else
			sTail = this;
		if (mPrev)
			mPrev->mNext = this;
		else
			sHead = this;
		mActive = true;
	}
}

void Renderable::Hide(void)
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

void Renderable::RenderAll(const AlignedBox2 &aView)
{
	// render matrix
	float angle;
	Vector2 position;

#ifdef RENDER_STATS
	// stats
	int drawn = 0, culled = 0;
#endif

	// render all renderables
	Renderable *itor = sHead;
	while (itor)
	{
		// get the next iterator
		// (in case the entry gets deleted)
		Renderable *next = itor->mNext;

		// get the entity (HACK)
		const Entity *entity = Database::entity.Get(itor->mId);
		if (!entity)
		{
			itor = next;
			continue;
		}

#ifdef RENDER_SIMULATION_POSITIONS
		// draw line between last and current simulated position
		glBegin(GL_LINES);
		glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
		glVertex2f(entity->GetPrevPosition().x, entity->GetPrevPosition().y);
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		glVertex2f(entity->GetPosition().x, entity->GetPosition().y);
		glEnd();
#endif
		// get interpolated position
		position = entity->GetInterpolatedPosition(sim_fraction);

		// if within the view area...
		if (position.x + itor->mRadius >= aView.min.x &&
			position.y + itor->mRadius >= aView.min.y &&
			position.x - itor->mRadius <= aView.max.x &&
			position.y - itor->mRadius <= aView.max.y)
		{
			// get interpolated angle
			angle = entity->GetInterpolatedAngle(sim_fraction);

			// get the renderable template
			const RenderableTemplate &renderable = Database::renderabletemplate.Get(itor->mId);

			// elapsed time
			float t = fmodf((int(sim_turn - itor->mStart) + sim_fraction - itor->mFraction) * sim_step, renderable.mPeriod);

			// render
			(itor->mAction)(itor->mId, t, renderable.mTransform ? Transform2(angle, position) : Transform2::Identity());

#ifdef RENDER_STATS
			++drawn;
#endif
		}
#ifdef RENDER_STATS
		else
		{
			++culled;
		}
#endif

		// go to the next iterator
		itor = next;
	}

#ifdef RENDER_STATS
	DebugPrint("d=%d/%d\n", drawn, drawn+culled);
#endif
}
