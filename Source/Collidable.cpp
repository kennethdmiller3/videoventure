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
: shapes(0)
{
}

CollidableTemplate::~CollidableTemplate(void)
{
}

bool CollidableTemplate::ProcessShapeItem(TiXmlElement *element, b2ShapeDef &shape)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &shape.localPosition.x);
		element->QueryFloatAttribute("y", &shape.localPosition.y);
		if (element->QueryFloatAttribute("angle", &shape.localRotation) == TIXML_SUCCESS)
			shape.localRotation *= float(M_PI)/180.0f;
		return true;

	case 0xa51be2bb /* "friction" */:
		element->QueryFloatAttribute("value", &shape.friction);
		return true;

	case 0xf59a4f8f /* "restitution" */:
		element->QueryFloatAttribute("value", &shape.restitution);
		return true;

	case 0x72b9059b /* "density" */:
		element->QueryFloatAttribute("value", &shape.density);
		return true;

	case 0xcf2f4271 /* "category" */:
		{
			int category = 0;
			element->QueryIntAttribute("value", &category);
			shape.categoryBits = 1<<category;
		}
		return true;

	case 0xe7774569 /* "mask" */:
		{
			char buf[16];
			for (int i = 0; i < 16; i++)
			{
				sprintf(buf, "bit%d", i);
				int bit = (shape.maskBits & (1 << i)) != 0;
				element->QueryIntAttribute(buf, &bit);
				if (bit)
					shape.maskBits |= (1 << i);
				else
					shape.maskBits &= ~(1 << i);
			}
		}
		return true;

	case 0x5fb91e8c /* "group" */:
		{
			int group = shape.groupIndex;
			element->QueryIntAttribute("value", &group);
			shape.groupIndex = short(group);
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureShape(TiXmlElement *element, b2ShapeDef &shape)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessShapeItem(child, shape);
	}
	return true;
}

bool CollidableTemplate::ConfigureCircle(TiXmlElement *element, b2CircleDef &shape)
{
	element->QueryFloatAttribute("radius", &shape.radius);
	ConfigureShape(element, shape);
	return true;
}

bool CollidableTemplate::ConfigureBox(TiXmlElement *element, b2BoxDef &shape)
{
	element->QueryFloatAttribute("w", &shape.extents.x);
	element->QueryFloatAttribute("h", &shape.extents.y);
	ConfigureShape(element, shape);
	return true;
}

bool CollidableTemplate::ProcessPolyItem(TiXmlElement *element, b2PolyDef &shape)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x945367a7 /* "vertex" */:
		element->QueryFloatAttribute("x", &shape.vertices[shape.vertexCount].x);
		element->QueryFloatAttribute("y", &shape.vertices[shape.vertexCount].y);
		++shape.vertexCount;
		return true;

	default:
		return ProcessShapeItem(element, shape);
	}
}

bool CollidableTemplate::ConfigurePoly(TiXmlElement *element, b2PolyDef &shape)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessPolyItem(child, shape);
	}
	return true;
}


bool CollidableTemplate::ProcessBodyItem(TiXmlElement *element, b2BodyDef &body)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &body.position.x);
		element->QueryFloatAttribute("y", &body.position.y);
		if (element->QueryFloatAttribute("angle", &body.rotation) == TIXML_SUCCESS)
			 body.rotation *= float(M_PI)/180.0f;
		return true;

	case 0x32741c32 /* "velocity" */:
		element->QueryFloatAttribute("x", &body.linearVelocity.x);
		element->QueryFloatAttribute("y", &body.linearVelocity.y);
		if (element->QueryFloatAttribute("angle", &body.angularVelocity) == TIXML_SUCCESS)
			body.angularVelocity *= float(M_PI)/180.0f;
		return true;

	case 0xbb61b895 /* "damping" */:
		element->QueryFloatAttribute("linear", &body.linearDamping);
		element->QueryFloatAttribute("angular", &body.angularDamping);
		return true;

	case 0xac01f355 /* "allowsleep" */:
		{
			int allowsleep = body.allowSleep;
			element->QueryIntAttribute("value", &allowsleep);
			body.allowSleep = allowsleep != 0;
		}
		return true;

	case 0xd233241f /* "preventrotation" */:
		{
			int preventrotation = body.preventRotation;
			element->QueryIntAttribute("value", &preventrotation);
			body.preventRotation = preventrotation != 0;
		}
		return true;

	case 0x28217089 /* "circle" */:
		{
			b2CircleDef *shape = new b2CircleDef;
			shapes.push_back(shape);
			ConfigureCircle(element, *shape);
			body.AddShape(shape);
		}
		return true;

	case 0x70c67e32 /* "box" */:
		{
			b2BoxDef *shape = new b2BoxDef;
			shapes.push_back(shape);
			ConfigureBox(element, *shape);
			body.AddShape(shape);
		}
		return true;

	case 0x84d6a947 /* "poly" */:
		{
			b2PolyDef *shape = new b2PolyDef;
			shapes.push_back(shape);
			ConfigurePoly(element, *shape);
			body.AddShape(shape);
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureBody(TiXmlElement *element, b2BodyDef &body)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessBodyItem(child, body);
	}
	return true;
}

bool CollidableTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x74e9dbae /* "collidable" */)
		return false;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *name = child->Value();
		switch (Hash(name))
		{
		case 0xdbaa7975 /* "body" */:
			{
				// add a body
				unsigned int id = Hash(child->Attribute("name"));
				b2BodyDef &body = bodies[id];
				CollidableTemplate::ConfigureBody(child, body);
			}
			break;

		case 0xaeae0877 /* "joint" */:
			break;
		}
	}

	return true;
}


b2World* Collidable::world;

Collidable::Collidable(void)
: CollidableTemplate()
{
}

Collidable::Collidable(const CollidableTemplate &aTemplate)
: CollidableTemplate(aTemplate), body(NULL)
{
}

Collidable::~Collidable(void)
{
	RemoveFromWorld();
}

void Collidable::AddToWorld(void)
{
	// for each body...
	for (BodyMap::iterator itor = bodies.begin(); itor != bodies.end(); ++itor)
	{
		// set body position to entity (HACK)
		b2BodyDef def(itor->second);
		def.userData = this;
		Entity *entity = dynamic_cast<Entity *>(this);
		if (entity)
		{
			const Matrix2 &transform = entity->GetTransform();
			def.rotation = transform.Angle();
			def.position = transform.p;
			def.linearVelocity = entity->GetVelocity();
			def.angularVelocity = 0.0f;
		}

		// create the body
		body = world->CreateBody(&def);
	}
}

void Collidable::RemoveFromWorld(void)
{
	if (body)
	{
		world->DestroyBody(body);
		body = NULL;
	}
}



// create collision world
void Collidable::WorldInit(void)
{
	// physics world
	b2AABB worldAABB;
	worldAABB.minVertex.Set(ARENA_X_MIN - 32, ARENA_Y_MIN - 32);
	worldAABB.maxVertex.Set(ARENA_X_MAX + 32, ARENA_Y_MAX + 32);
	b2Vec2 gravity;
	gravity.Set(0.0f, 0.0f);
	bool doSleep = true;
	world = new b2World(worldAABB, gravity, doSleep);

	// create perimeter walls
	b2BodyDef body;

	b2BoxDef top;
	top.localPosition.Set(0.5f * (ARENA_X_MAX + ARENA_X_MIN), ARENA_Y_MIN - 16);
	top.extents.Set(0.5f * (ARENA_X_MAX - ARENA_X_MIN) + 32, 16);
	body.AddShape(&top);

	b2BoxDef bottom;
	bottom.localPosition.Set(0.5f * (ARENA_X_MAX + ARENA_X_MIN), ARENA_Y_MAX + 16);
	bottom.extents.Set(0.5f * (ARENA_X_MAX - ARENA_X_MIN) + 32, 16);
	body.AddShape(&bottom);

	b2BoxDef left;
	left.localPosition.Set(ARENA_X_MIN - 16, 0.5f * (ARENA_Y_MAX + ARENA_Y_MIN));
	left.extents.Set(16, 0.5f * (ARENA_Y_MAX - ARENA_Y_MIN) + 32);
	body.AddShape(&left);

	b2BoxDef right;
	right.localPosition.Set(ARENA_X_MAX + 16, 0.5f * (ARENA_Y_MAX + ARENA_Y_MIN));
	right.extents.Set(16, 0.5f * (ARENA_Y_MAX - ARENA_Y_MIN) + 32);
	body.AddShape(&right);

	world->CreateBody(&body);
}

void Collidable::WorldDone(void)
{
	delete world;
}

void Collidable::CollideAll(float aStep)
{
//	aStep *= (1.0f/16.0f);
//	for (int i = 0; i < 16; i++)
	{
		world->Step(aStep, 16);
		world->m_broadPhase->Validate();

		// for each body...
		for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
		{
			if (!b->IsSleeping() && !b->IsStatic())
			{
				// update the entity position (hack)
				Entity *entity = dynamic_cast<Entity *>(static_cast<Collidable *>(b->GetUserData()));
				if (entity)
				{
					entity->Step();
					entity->SetTransform(Matrix2(b->GetRotationMatrix(), b->GetOriginPosition()));
					entity->SetVelocity(Vector2(b->GetLinearVelocity()));
				}
			}
		}

		// for each contact...
		for (b2Contact* c = world->GetContactList(); c; c = c->GetNext())
		{
			// if the shapes are actually touching...
			if (c->GetManifoldCount() > 0)
			{
				b2Body* body1 = c->GetShape1()->GetBody();
				b2Body* body2 = c->GetShape2()->GetBody();
				Collidable* coll1 = static_cast<Collidable *>(body1->GetUserData());
				Collidable* coll2 = static_cast<Collidable *>(body2->GetUserData());
				if (coll1 && coll2)
				{
					coll1->Collide(*coll2, c->GetManifolds(), c->GetManifoldCount());
					coll2->Collide(*coll1, c->GetManifolds(), c->GetManifoldCount());
				}
			}
		}
	}
}
