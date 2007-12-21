#include "StdAfx.h"
#include "Explosion.h"
#include <boost/pool/pool.hpp>

namespace Database
{
	Typed<ExplosionTemplate> explosiontemplate("explosiontemplate");
	Typed<Explosion *> explosion("explosion");
};


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
: ExplosionTemplate()
, Simulatable(0)
, Renderable()
, mLife(0)
{
}

Explosion::Explosion(const ExplosionTemplate &aTemplate, unsigned int aId)
: ExplosionTemplate(aTemplate)
, Simulatable(aId)
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
	// elapsed time
	float t = mLifeSpan - mLife + Renderable::sOffset;

	// get the explosion template
	const ExplosionTemplate &explosion = Database::explosiontemplate.Get(Simulatable::id);

	// execute the deferred draw list
	ExecuteDeferredDrawItems(&explosion.mBuffer[0], explosion.mBuffer.size(), t);
}
