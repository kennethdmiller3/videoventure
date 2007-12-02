#include "StdAfx.h"
#include "Collidable.h"
#include "Entity.h"
#include <typeinfo>

namespace Database
{
	Typed<CollidableTemplate> collidabletemplate("collidabletemplate");
	Typed<Collidable> collidable("collidable");
}

CollidableTemplate::CollidableTemplate(void)
: layer(-1), type(TYPE_NONE), size(0, 0)
{
}

CollidableTemplate::~CollidableTemplate(void)
{
}

bool CollidableTemplate::Attribute(TiXmlAttribute *attrib)
{
	const char *label = attrib->Name();
	switch (Hash(label))
	{
	case 0x07a640f6 /* "layer" */:
		layer = attrib->IntValue();
		return true;

	case 0x5127f14d /* "type" */:
		switch (Hash(attrib->Value()))
		{
		case 0x06dbc8c0 /* "alignedbox" */:
			type = Collidable::TYPE_ALIGNED_BOX;
			break;
		case 0x28217089 /* "circle" */:
			type = Collidable::TYPE_CIRCLE;
			break;
		default:
			type = Collidable::TYPE_NONE;
			break;
		}
		return true;

	case 0x0dba4cb3 /* "radius" */:
		size.x = size.y = float(attrib->DoubleValue());
		return true;

	case 0x95876e1f /* "width" */:
		size.x = float(attrib->DoubleValue());
		return true;

	case 0xd5bdbb42 /* "height" */:
		size.y = float(attrib->DoubleValue());
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x74e9dbae /* "collidable" */)
		return false;

	// process child elements
	for (TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
	{
		Attribute(attrib);
	}

	return true;
}


Collidable::List Collidable::sLayer[COLLISION_LAYERS];
unsigned int Collidable::sLayerMask[COLLISION_LAYERS];

Collidable::Collidable(void)
: CollidableTemplate()
{
}

Collidable::Collidable(const CollidableTemplate &aTemplate)
: CollidableTemplate(aTemplate)
{
	int aLayer = layer;
	layer = -1;
	SetLayer(aLayer);
}

Collidable::~Collidable(void)
{
	SetLayer(-1);
}

void Collidable::SetLayer(int aLayer)
{
	if (layer != aLayer)
	{
		if (layer >= 0 && layer < COLLISION_LAYERS)
			sLayer[layer].erase(entry);

		layer = aLayer;

		if (layer >= 0 && layer < COLLISION_LAYERS)
			entry = sLayer[layer].insert(sLayer[layer].end(), this);
		else
			entry = List::iterator();
	}
}

bool Collidable::Attribute(TiXmlAttribute *attrib)
{
	const char *label = attrib->Name();
	switch (Hash(label))
	{
	case 0x07a640f6 /* "layer" */:
		SetLayer(attrib->IntValue());
		return true;

	default:
		return CollidableTemplate::Attribute(attrib);
	}
}

static bool TestAlignedAligned(const AlignedBox2 &a1, const AlignedBox2 &a2)
{
	return
		a1.min.x >= a2.max.x &&
		a1.min.y >= a2.max.y &&
		a1.max.x <= a2.min.x &&
		a1.max.y <= a2.min.y;
}

static bool TestRayAligned(float &aStep, const AlignedBox2 &a, const Vector2 &p, const Vector2 &v)
{
	// Smits’ method
	//bool Box::intersect(const Ray &r, float t0, float t1) const {
	float tmin, tmax, tymin, tymax;
	if (v.x >= 0) {
		tmin = (a.min.x - p.x) / v.x;
		tmax = (a.max.x - p.x) / v.x;
	}
	else {
		tmin = (a.max.x - p.x) / v.x;
		tmax = (a.min.x - p.x) / v.x;
	}
	if (v.y >= 0) {
		tymin = (a.min.y - p.y) / v.y;
		tymax = (a.max.y - p.y) / v.y;
	}
	else {
		tymin = (a.max.y - p.y) / v.y;
		tymax = (a.min.y - p.y) / v.y;
	}
	if ( (tmin > tymax) || (tymin > tmax) )
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	if ( tmin < aStep && tmax > 0.0f )
	{
		if (tmin > 0.0f)
			aStep = tmin;
		else
			aStep = 0.0f;
		return true;
	}
	return false;
}

#if 0
static bool TestAlignedCircle(const AlignedBox2 &a,
							  const Sphere2 &c)
{
	float d = 0.0f;
	if ( c.p.x < a.min.x)
	{
		float s = c.p.x - a.min.x;
		d += s * s;
	}
	else if ( c.p.x > a.max.x)
	{
		float s = c.p.x - a.max.x;
		d += s * s;
	}
	if ( c.p.y < a.min.y)
	{
		float s = c.p.y - a.min.y;
		d += s * s;
	}
	else if ( c.p.y > a.max.y)
	{
		float s = c.p.y - a.max.y;
		d += s * s;
	}
	return d <= c.r * c.r;
}
#endif

static bool TestRayCircle(float &aStep, const Vector2 &p, const Vector2 &v, float r)
{
	// c = dP . dP - R^2
	float pp = p.Dot(p) - r * r;
	if ( pp < 0 )
	{
		// start overlapping
		aStep = 0.0f;
		return true;
	}

	// b = 2 ( dP . dV )
	float pv = p.Dot(v);
	if ( pv >= 0 )
	{
		// moving apart
		return false;
	}

	// a = dV . dV
	float vv = v.Dot(v);

	// normalize interval to [0..1]
	vv *= aStep * aStep;
	pv *= aStep;

	// if further than step
	if ( (pv + vv) <= 0 && (vv + 2 * pv + pp) >= 0 )
	{
		// trivial reject
		return false;
	}

	// time of closest approach
	float tmp = -pv / vv;

	// D = pv * pv - vv * pp;
	// tmp2 = -D / (4 * vv)
	float tmp2 = pp + pv * tmp;

	// check discriminant
	if (tmp2 > 0)
	{
		// never intersect
		return false;
	}

	// calculate the time to intersection
	float t = tmp - sqrtf(-tmp2 / vv);
	if (t < 1)
	{
		// return time to intersection
		aStep *= t;
		return true;
	}
	return false;
}

static bool TestCircleCircle(float &aStep,
							 const Sphere2 &c1, const Vector2 &v1,
							 const Sphere2 &c2, const Vector2 &v2)
{
	return TestRayCircle(aStep, c2.p - c1.p, v2 - v1, c2.r + c1.r);
}

void Collidable::CollideAll(float aStep)
{
	// for each collision layer...
	for (int i = 0; i < COLLISION_LAYERS; i++)
	{
		// skip noncolliding layers
		if (sLayerMask[i] == 0)
			continue;

		// for each entry in the collision layer...
		List::iterator itor1 = sLayer[i].begin();
		while (itor1 != sLayer[i].end())
		{
			// get the next iterator
			List::iterator next1(itor1);
			++next1;

			// get the collidable
			Collidable *coll1 = *itor1;
			Entity *ent1 = dynamic_cast<Entity *>(coll1);

			// hack!
			coll1->box.min = ent1->GetPosition() - coll1->size;
			coll1->box.max = ent1->GetPosition() + coll1->size;

			// intersection time
			float t = aStep;
			Collidable *coll = NULL;

			// for each subesquent layer...
			for (int j = 0; j < COLLISION_LAYERS; j++)
			{
				// skip non-interacting layers
				if (!((sLayerMask[i] >> j) & 1))
					continue;

				// for each entry in the recipient layer...
				List::iterator itor2 = sLayer[j].begin();
				while (itor2 != sLayer[j].end())
				{
					// get the next iterator
					List::iterator next2(itor2);
					++next2;

					// get the collidable
					Collidable *coll2 = *itor2;
					Entity *ent2 = dynamic_cast<Entity *>(coll2);

					// hack!
					coll2->box.min = ent2->GetPosition() - coll2->size;
					coll2->box.max = ent2->GetPosition() + coll2->size;

					switch(coll1->type)
					{
					case TYPE_ALIGNED_BOX:
						switch(coll2->type)
						{
						case TYPE_ALIGNED_BOX:
							if (TestAlignedAligned(coll1->box, coll2->box))
							{
								t = 0.0f;
								coll = coll2;
							}
							break;

						case TYPE_CIRCLE:
							//if (TestAlignedCircle(coll1->box, Sphere2(ent2->GetPosition(), coll2->size.x)))
							if (TestRayAligned(t,
								AlignedBox2(coll1->box.min - coll2->size, coll1->box.max + coll2->size),
								ent2->GetPosition(),
								ent2->GetVelocity() - ent1->GetVelocity()))
							{
								coll = coll2;
							}
							break;

						default:
							break;
						}
						break;

					case TYPE_CIRCLE:
						switch(coll2->type)
						{
						case TYPE_ALIGNED_BOX:
							//if (TestAlignedCircle(coll2->box, Sphere2(ent1->GetPosition(), coll1->size.x)))
							if (TestRayAligned(t,
								AlignedBox2(coll2->box.min - coll1->size, coll2->box.max + coll1->size),
								ent1->GetPosition(),
								ent1->GetVelocity() - ent2->GetVelocity()))
							{
								coll = coll2;
							}
							break;

						case TYPE_CIRCLE:
							if (TestCircleCircle(t,
								Sphere2(ent1->GetPosition(), coll1->size.x), ent1->GetVelocity(),
								Sphere2(ent2->GetPosition(), coll2->size.x), ent2->GetVelocity()))
							{
								coll = coll2;
							}
							break;

						default:
							break;
						}
						break;

					default:
						break;
					}

					// go to the next iterator
					itor2 = next2;
				}
			}

			// if collided...
			if (coll)
			{
#ifdef DEBUG_TRACE_COLLISION
				Entity *ent2 = dynamic_cast<Entity *>(coll);
				DebugPrint("%s %d collided with %s %d at %f\n",
					typeid(*ent1).name(), ent1->GetId(),
					typeid(*ent2).name(), ent2->GetId(),
					t);
#endif
				// advance to contact point
				ent1->SetPosition(ent1->GetPosition() + ent1->GetVelocity() * t);

				// notify of collision
				coll1->Collide(t, *coll);
			}

			// go to the next iterator
			itor1 = next1;
		}
	}
}
