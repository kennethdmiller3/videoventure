#include "StdAfx.h"
#include "Explosion.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// explosion pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Explosion));
void *Explosion::operator new(size_t aSize)
{
	return pool.malloc();
}
void Explosion::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<ExplosionTemplate> explosiontemplate(0xbde38dea /* "explosiontemplate" */);
	Typed<Explosion *> explosion(0x02bb1fe0 /* "explosion" */);

	namespace Initializer
	{
		class ExplosionInitializer
		{
		public:
			ExplosionInitializer()
			{
				AddActivate(0xbde38dea /* "explosiontemplate" */, Entry(this, &ExplosionInitializer::Activate));
				AddDeactivate(0xbde38dea /* "explosiontemplate" */, Entry(this, &ExplosionInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const ExplosionTemplate &explosiontemplate = Database::explosiontemplate.Get(aId);
				Explosion *explosion = new Explosion(explosiontemplate, aId);
				Database::explosion.Put(aId, explosion);
			}

			void Deactivate(unsigned int aId)
			{
				if (Explosion *explosion = Database::explosion.Get(aId))
				{
					delete explosion;
					Database::explosion.Delete(aId);
				}
			}
		}
		explosioninitializer;
	}
};


ExplosionTemplate::ExplosionTemplate(void)
: mLifeSpan( 0.25f )
{
}

ExplosionTemplate::~ExplosionTemplate(void)
{
}


bool ExplosionTemplate::Configure(TiXmlElement *element, unsigned int id)
{
	if (Hash(element->Value()) != 0x02bb1fe0 /* "explosion" */)
		return false;

	element->QueryFloatAttribute("life", &mLifeSpan);

	int coreindex = 0;
	int haloindex = 0;

	// process child elements
	ProcessDrawItemsDeferred(element, id, mBuffer);

	return true;
}


Explosion::Explosion(void)
: Simulatable(0)
, Renderable()
, mLife(0)
{
}

Explosion::Explosion(const ExplosionTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
, Renderable(RenderableTemplate(), aId)
, mLife(aTemplate.mLifeSpan)
{
	// add as a renderable (HACK)
	Database::renderable.Put(Simulatable::id, this);

	// set as visible
	Renderable::Show();
}

Explosion::~Explosion(void)
{
	// remove as a renderable (HACK)
	Database::renderable.Delete(Simulatable::id);
}

void Explosion::Simulate(float aStep)
{
	// advance life timer
	mLife -= aStep;
	if (mLife <= 0)
	{
		Database::Delete(Simulatable::id);
		return;
	}
}

void Explosion::Render(const Matrix2 &transform)
{
	// get the explosion template
	const ExplosionTemplate &explosion = Database::explosiontemplate.Get(Simulatable::id);

	// elapsed time
	float t = explosion.mLifeSpan - mLife + Renderable::sOffset;

	// execute the deferred draw list
	ExecuteDeferredDrawItems(&explosion.mBuffer[0], explosion.mBuffer.size(), t);
}
