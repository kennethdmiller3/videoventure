#include "stdafx.h"

#include "Updatable.h"
#include "Collidable.h"
#include "Entity.h"

class TraceTemplate
{
public:
	CollidableFilter mFilter;
	float mLinearDamping;
	float mAngularDamping;

public:
	TraceTemplate();

	void Configure(const tinyxml2::XMLElement *element, unsigned int id);
};

class Trace : public Updatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Trace(const TraceTemplate &aTemplate, unsigned int aId);

	void Update(float aStep);
};



#ifdef USE_POOL_ALLOCATOR

// particle pool
static MemoryPool sPool(sizeof(Trace));
void *Trace::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Trace::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


extern void ConfigureFilterData(CollidableFilter &aFilter, const tinyxml2::XMLElement *element);


namespace Database
{
	Typed<TraceTemplate> tracetemplate(0x9e24e600 /* "tracetemplate" */);
	Typed<Trace *> trace(0x813d75ae /* "trace" */);

	namespace Loader
	{
		static void TraceConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			TraceTemplate &trace = Database::tracetemplate.Open(aId);
			trace.Configure(element, aId);
			Database::tracetemplate.Close(aId);
		}
		Configure traceconfigure(0x813d75ae /* "trace" */, TraceConfigure);
	}

	namespace Initializer
	{
		static void TraceActivate(unsigned int aId)
		{
			const TraceTemplate &tracetemplate = Database::tracetemplate.Get(aId);
			Trace *trace = new Trace(tracetemplate, aId);
			Database::trace.Put(aId, trace);
		}
		Activate traceactivate(0x9e24e600 /* "tracetemplate" */, TraceActivate);

		static void TraceDeactivate(unsigned int aId)
		{
			if (Trace *trace = Database::trace.Get(aId))
			{
				delete trace;
				Database::trace.Delete(aId);
			}
		}
		Deactivate tracedeactivate(0x9e24e600 /* "tracetemplate" */, TraceDeactivate);
	}
}


TraceTemplate::TraceTemplate(void)
	: mFilter()
	, mLinearDamping(0.0f)
	, mAngularDamping(0.0f)
{
}

void TraceTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int id)
{
	ConfigureFilterData(mFilter, element);

	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0xbb61b895 /* "damping" */:
			child->QueryFloatAttribute("linear", &mLinearDamping);
			child->QueryFloatAttribute("angular", &mAngularDamping);
		}
	}
}

Trace::Trace(const TraceTemplate &aTemplate, unsigned int aId)
	: Updatable(aId)
{
	SetAction(Action(this, &Trace::Update));
	Activate();
}

void Trace::Update(float aStep)
{
	if (Entity *entity = Database::entity.Get(mId))
	{
		const TraceTemplate &trace = Database::tracetemplate.Get(mId);

		entity->Step();

		// apply damping
		entity->SetVelocity(entity->GetVelocity() * (1.0f - trace.mLinearDamping * aStep));
		entity->SetOmega(entity->GetOmega() * (1.0f - trace.mAngularDamping * aStep));

		// get start and end points
		Transform2 start(entity->GetTransform());
		Transform2 end(start.a + aStep * entity->GetOmega(), start.p + aStep * entity->GetVelocity());

		// perform segment test
		float lambda;
		Vector2 normal;
		CollidableShape *shape;
		unsigned int hitId = Collidable::TestSegment(start.p, end.p, trace.mFilter, mId, lambda, normal, shape);
		
		// if the segment hit something...
		if (lambda < 1.0f)
		{
			// find point of impact
			end.p = Lerp(start.p, end.p, lambda);
			end.a = Lerp(start.a, end.a, lambda);

			// move to the point of impact
			entity->SetTransform(end);

			// signal contact add
			Database::collidablecontactadd.Get(mId)(mId, hitId, 0.0f, end.p, normal);
			Database::collidablecontactadd.Get(hitId)(hitId, mId, 0.0f, end.p, normal);
		}
		else
		{
			// move to the end point
			entity->SetTransform(end);
		}
	}
}
