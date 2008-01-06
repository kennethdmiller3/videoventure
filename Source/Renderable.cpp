#include "StdAfx.h"
#include "Renderable.h"
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
	Typed<GLuint> drawlist(0xc98b019b /* "drawlist" */);

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
: mPeriod(FLT_MAX)
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

	// animation period
	element->QueryFloatAttribute("period", &mPeriod);

	// process child elements
	ProcessDrawItemsDeferred(element, mBuffer);

	return true;
}



Renderable::List Renderable::sAll;
unsigned int Renderable::sTurn;
float Renderable::sOffset;

Renderable::Renderable(void)
: id(0), show(false), mStart(sTurn), mFraction(0.0f)
{
}

Renderable::Renderable(const RenderableTemplate &aTemplate, unsigned int aId)
: id(aId), show(false), mStart(sTurn), mFraction(0.0f)
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

void Renderable::RenderAll(float aRatio, float aStep)
{
	// compute offset between visible time and simulated time
	sOffset = (aRatio - 1.0f);

	// render matrix
	Matrix2 transform;

	// render all renderables
	List::iterator itor = sAll.begin();
	while (itor != sAll.end())
	{
		// get the next iterator
		// (in case the entry gets deleted)
		List::iterator next(itor);
		++next;

		// push a transform
		glPushMatrix();

		// get the entity (HACK)
		const Entity *entity = Database::entity.Get((*itor)->id);
		if (entity)
		{
			// get interpolated transform
			transform = entity->GetInterpolatedTransform(aRatio);

			// load matrix
			float m[16] =
			{
				transform.x.x, transform.x.y, 0, 0,
				transform.y.x, transform.y.y, 0, 0,
				0, 0, 1, 0,
				transform.p.x, transform.p.y, 0, 1
			};
			glMultMatrixf( m );
		}
		else
		{
			// use identity matrix
			transform = Matrix2(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0));
		}

		// render
		(*itor)->Render(transform, aStep);

		// reset the transform
		glPopMatrix();

		// go to the next iterator
		itor = next;
	}
}

void Renderable::Render(const Matrix2 &aTransform, float aStep)
{
	// get the renderable template
	const RenderableTemplate &renderable = Database::renderabletemplate.Get(id);

	// elapsed time
	float t = fmodf((sTurn - mStart + sOffset - mFraction) * aStep, renderable.mPeriod);
	if (t < 0)
		return;

	// execute the deferred draw list
	ExecuteDeferredDrawItems(&renderable.mBuffer[0], renderable.mBuffer.size(), t);
};
