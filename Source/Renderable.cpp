#include "StdAfx.h"
#include "Renderable.h"
#include "Drawlist.h"
#include "Entity.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// renderable pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Renderable));
void *Renderable::operator new(size_t aSize)
{
	return pool.malloc();
}
void Renderable::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<RenderableTemplate> renderabletemplate(0x0cb54133 /* "renderabletemplate" */);
	Typed<Renderable *> renderable(0x109dd1ad /* "renderable" */);

	namespace Loader
	{
		class RenderableLoader
		{
		public:
			RenderableLoader()
			{
				AddConfigure(0x109dd1ad /* "renderable" */, Entry(this, &RenderableLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				RenderableTemplate &renderable = Database::renderabletemplate.Open(aId);
				renderable.Configure(element, aId);
				Database::renderabletemplate.Close(aId);
			}
		}
		renderableloader;
	}

	namespace Initializer
	{
		class RenderableInitializer
		{
		public:
			RenderableInitializer()
			{
				AddActivate(0x0cb54133 /* "renderabletemplate" */, Entry(this, &RenderableInitializer::Activate));
				AddDeactivate(0x0cb54133 /* "renderabletemplate" */, Entry(this, &RenderableInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const RenderableTemplate &renderabletemplate = Database::renderabletemplate.Get(aId);
				Renderable *renderable = new Renderable(renderabletemplate, aId);
				Database::renderable.Put(aId, renderable);
				renderable->Show();
			}

			void Deactivate(unsigned int aId)
			{
				if (Renderable *renderable = Database::renderable.Get(aId))
				{
					renderable->Hide();
					delete renderable;
					Database::renderable.Delete(aId);
				}
			}
		}
		renderableinitializer;
	}
}

RenderableTemplate::RenderableTemplate(void)
: mRadius(0)
, mPeriod(FLT_MAX)
{
}

RenderableTemplate::~RenderableTemplate(void)
{
}

// configure
bool RenderableTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	if (Hash(element->Value()) != 0x109dd1ad /* "renderable" */)
		return false;

	// animation period
	element->QueryFloatAttribute("period", &mPeriod);

	// bounds
//	element->QueryFloatAttribute("x", &mBounds.p.x);
//	element->QueryFloatAttribute("y", &mBounds.p.y);
	element->QueryFloatAttribute("radius", &mRadius);

	// process child elements
//	ProcessDrawItems(element, mBuffer);
	std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Open(aId);
	ProcessDrawItems(element, buffer);
	Database::dynamicdrawlist.Close(aId);

	return true;
}



Renderable *Renderable::sHead;
Renderable *Renderable::sTail;
unsigned int Renderable::sTurn;
float Renderable::sOffset;

Renderable::Renderable(void)
: mId(0), mNext(NULL), mPrev(NULL), show(false), mRadius(0), mStart(sTurn), mFraction(0.0f)
{
}

Renderable::Renderable(const RenderableTemplate &aTemplate, unsigned int aId)
: mId(aId), mNext(NULL), mPrev(NULL), show(false), mRadius(aTemplate.mRadius), mStart(sTurn), mFraction(0.0f)
{
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
		mNext = sHead;
		if (sHead)
			sHead->mPrev = this;
		sHead = this;
		if (!sTail)
			sTail = this;
#else
		mPrev = sTail;
		if (sTail)
			sTail->mNext = this;
		sTail = this;
		if (!sHead)
			sHead = this;
#endif
		show = true;
	}
}

void Renderable::Hide(void)
{
	if (show)
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
		show = false;
	}
}

void Renderable::RenderAll(float aRatio, float aStep, const AlignedBox2 &aView)
{
	// compute offset between visible time and simulated time
	sOffset = (aRatio - 1.0f);

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
			continue;

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
		position = entity->GetInterpolatedPosition(aRatio);

		// if within the view area...
		if (position.x + itor->mRadius >= aView.min.x &&
			position.y + itor->mRadius >= aView.min.y &&
			position.x - itor->mRadius <= aView.max.x &&
			position.y - itor->mRadius <= aView.max.y)
		{
			// get interpolated angle
			angle = entity->GetInterpolatedAngle(aRatio);

			// render
			itor->Render(aStep, position.x, position.y, angle);

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

void Renderable::Render(float aStep, float aPosX, float aPosY, float aAngle)
{
	// get the renderable template
	const RenderableTemplate &renderable = Database::renderabletemplate.Get(mId);

	// elapsed time
	float t = fmodf((sTurn - mStart + sOffset - mFraction) * aStep, renderable.mPeriod);

	// skip if not visible
	if (t < 0)
		return;

	// get the dynamic draw list
	const std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Get(mId);

	// skip if empty
	if (buffer.empty())
		return;

	// push a transform
	glPushMatrix();

	// load matrix
	glTranslatef(aPosX, aPosY, 0);
	glRotatef(aAngle*180/float(M_PI), 0.0f, 0.0f, 1.0f);

	// execute the deferred draw list
	ExecuteDrawItems(&buffer[0], buffer.size(), t, mId);

	// reset the transform
	glPopMatrix();
};
