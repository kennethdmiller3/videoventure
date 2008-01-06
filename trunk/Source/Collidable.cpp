#include "StdAfx.h"
#include "Collidable.h"
#include "Entity.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// collidable pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Collidable));
void *Collidable::operator new(size_t aSize)
{
	return pool.malloc();
}
void Collidable::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<CollidableTemplate> collidabletemplate(0xa7380c00 /* "collidabletemplate" */);
	Typed<Typed<b2BodyDef> > collidabletemplatebody(0x66727d0c /* "collidabletemplatebody" */);
	Typed<Collidable *> collidable(0x74e9dbae /* "collidable" */);
	Typed<Typed<b2Body *> > collidablebody(0x6ccc2b62 /* "collidablebody" */);
	Typed<Typed<Collidable::Listener> > collidablelistener(0xf4c15fb2 /* "collidablelistener" */);

	namespace Initializer
	{
		class CollidableInitializer
		{
		public:
			CollidableInitializer()
			{
				AddActivate(0xa7380c00 /* "collidabletemplate" */, Entry(this, &CollidableInitializer::Activate));
				AddDeactivate(0xa7380c00 /* "collidabletemplate" */, Entry(this, &CollidableInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const CollidableTemplate &collidabletemplate = Database::collidabletemplate.Get(aId);
				Collidable *collidable = new Collidable(collidabletemplate, aId);
				Database::collidable.Put(aId, collidable);
				collidable->AddToWorld();
			}

			void Deactivate(unsigned int aId)
			{
				if (Collidable *collidable = Database::collidable.Get(aId))
				{
					collidable->RemoveFromWorld();
					delete collidable;
					Database::collidable.Delete(aId);
					Database::collidablelistener.Delete(aId);
				}
			}
		}
		collidableinitializer;
	}
}

CollidableTemplate::CollidableTemplate(void)
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

	case 0x029402af /* "fast" */:
		{
			int isBullet = body.isBullet;
			element->QueryIntAttribute("value", &isBullet);
			body.isBullet = isBullet != 0;
		}
		return true;

	case 0x28217089 /* "circle" */:
		{
			circles.push_back(b2CircleDef());
			ConfigureCircle(element, circles.back());
			body.AddShape(&circles.back());
		}
		return true;

	case 0x70c67e32 /* "box" */:
		{
			boxes.push_back(b2BoxDef());
			ConfigureBox(element, boxes.back());
			body.AddShape(&boxes.back());
		}
		return true;

	case 0x84d6a947 /* "poly" */:
		{
			polys.push_back(b2PolyDef());
			ConfigurePoly(element, polys.back());
			body.AddShape(&polys.back());
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

bool CollidableTemplate::ProcessJointItem(TiXmlElement *element, b2JointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x115ce60c /* "body1" */:
		{
			const char *name = element->Attribute("name");
			const char *body = element->Attribute("body");
			JointTemplate *data = static_cast<JointTemplate *>(joint.userData);
			data->name1 = name ? Hash(name) : 0;
			data->body1 = body ? Hash(body) : 0;
		}
		return true;

	case 0x145ceac5 /* "body2" */:
		{
			const char *name = element->Attribute("name");
			const char *body = element->Attribute("body");
			JointTemplate *data = static_cast<JointTemplate *>(joint.userData);
			data->name2 = name ? Hash(name) : 0;
			data->body2 = body ? Hash(body) : 0;
		}
		return true;

	case 0x2c5d8028 /* "collideconnected" */:
		{
			int collide = joint.collideConnected;
			element->QueryIntAttribute("value", &collide);
			joint.collideConnected = collide != 0;
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ProcessRevoluteJointItem(TiXmlElement *element, b2RevoluteJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x42edcab4 /* "anchor" */:
		element->QueryFloatAttribute("x", &joint.anchorPoint.x);
		element->QueryFloatAttribute("y", &joint.anchorPoint.y);
		return true;

	case 0x32dad934 /* "limit" */:
		element->QueryFloatAttribute("lower", &joint.lowerAngle);
		element->QueryFloatAttribute("upper", &joint.upperAngle);
		joint.enableLimit = true;
		return true;

	case 0xcaf08472 /* "motor" */:
		element->QueryFloatAttribute("torque", &joint.motorTorque);
		element->QueryFloatAttribute("speed", &joint.motorSpeed);
		joint.enableMotor = true;
		return true;

	default:
		return ProcessJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigureRevoluteJoint(TiXmlElement *element, b2RevoluteJointDef &joint)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessRevoluteJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ProcessPrismaticJointItem(TiXmlElement *element, b2PrismaticJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x42edcab4 /* "anchor" */:
		element->QueryFloatAttribute("x", &joint.anchorPoint.x);
		element->QueryFloatAttribute("y", &joint.anchorPoint.y);
		return true;

	case 0x6d2badf4 /* "axis" */:
		element->QueryFloatAttribute("x", &joint.axis.x);
		element->QueryFloatAttribute("y", &joint.axis.y);
		return true;

	case 0x32dad934 /* "limit" */:
		element->QueryFloatAttribute("lower", &joint.lowerTranslation);
		element->QueryFloatAttribute("upper", &joint.upperTranslation);
		joint.enableLimit = true;
		return true;

	case 0xcaf08472 /* "motor" */:
		element->QueryFloatAttribute("force", &joint.motorForce);
		element->QueryFloatAttribute("speed", &joint.motorSpeed);
		joint.enableMotor = true;
		return true;

	default:
		return ProcessJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigurePrismaticJoint(TiXmlElement *element, b2PrismaticJointDef &joint)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessPrismaticJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ProcessDistanceJointItem(TiXmlElement *element, b2DistanceJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.anchorPoint1.x);
		element->QueryFloatAttribute("y", &joint.anchorPoint1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.anchorPoint2.x);
		element->QueryFloatAttribute("y", &joint.anchorPoint2.y);
		return true;

	default:
		return ProcessJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigureDistanceJoint(TiXmlElement *element, b2DistanceJointDef &joint)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessDistanceJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ProcessPulleyJointItem(TiXmlElement *element, b2PulleyJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe1acc15d /* "ground1" */:
		element->QueryFloatAttribute("x", &joint.groundPoint1.x);
		element->QueryFloatAttribute("y", &joint.groundPoint1.y);
		return true;

	case 0xdeacbca4 /* "ground2" */:
		element->QueryFloatAttribute("x", &joint.groundPoint1.x);
		element->QueryFloatAttribute("y", &joint.groundPoint1.y);
		return true;

	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.anchorPoint1.x);
		element->QueryFloatAttribute("y", &joint.anchorPoint1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.anchorPoint2.x);
		element->QueryFloatAttribute("y", &joint.anchorPoint2.y);
		return true;

	case 0xa4c53aac /* "length1" */:
		element->QueryFloatAttribute("max", &joint.maxLength1);
		return true;

	case 0xa7c53f65 /* "length2" */:
		element->QueryFloatAttribute("max", &joint.maxLength2);
		return true;

	case 0xc1121e84 /* "ratio" */:
		element->QueryFloatAttribute("value", &joint.ratio);
		return true;

	default:
		return ProcessJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigurePulleyJoint(TiXmlElement *element, b2PulleyJointDef &joint)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessPulleyJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ProcessMouseJointItem(TiXmlElement *element, b2MouseJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x32608848 /* "target" */:
		element->QueryFloatAttribute("x", &joint.target.x);
		element->QueryFloatAttribute("y", &joint.target.y);
		return true;

	case 0x79a98884 /* "force" */:
		element->QueryFloatAttribute("max", &joint.maxForce);
		return true;

	case 0x78e63274 /* "tuning" */:
		element->QueryFloatAttribute("frequency", &joint.frequencyHz);
		element->QueryFloatAttribute("damping", &joint.dampingRatio);
		element->QueryFloatAttribute("timestep", &joint.timeStep);
		return true;

	default:
		return ProcessJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigureMouseJoint(TiXmlElement *element, b2MouseJointDef &joint)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessMouseJointItem(child, joint);
	}
	return true;
}


bool CollidableTemplate::Configure(TiXmlElement *element, unsigned int id)
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
				unsigned int bodyid = Hash(child->Attribute("name"));
				Database::Typed<b2BodyDef> &bodies = Database::collidabletemplatebody.Open(id);
				b2BodyDef &body = bodies.Open(bodyid);
				CollidableTemplate::ConfigureBody(child, body);
				bodies.Close(bodyid);
				Database::collidabletemplatebody.Close(id);
			}
			break;

		case 0xef2f9539 /* "revolutejoint" */:
			{
				revolutes.push_back(b2RevoluteJointDef());
				b2RevoluteJointDef &joint = revolutes.back();
				joints.push_back(JointTemplate());
				joint.userData = &joints.back();
				CollidableTemplate::ConfigureRevoluteJoint(child, joint);
			}
			break;

		case 0x4954853d /* "prismaticjoint" */:
			{
				prismatics.push_back(b2PrismaticJointDef());
				b2PrismaticJointDef &joint = prismatics.back();
				joints.push_back(JointTemplate());
				joint.userData = &joints.back();
				CollidableTemplate::ConfigurePrismaticJoint(child, joint);
			}
			break;

		case 0x6932d1ee /* "distancejoint" */:
			{
				distances.push_back(b2DistanceJointDef());
				b2DistanceJointDef &joint = distances.back();
				joints.push_back(JointTemplate());
				joint.userData = &joints.back();
				CollidableTemplate::ConfigureDistanceJoint(child, joint);
			}
			break;

		case 0xdd003dc4 /* "pulleyjoint" */:
			{
				pulleys.push_back(b2PulleyJointDef());
				b2PulleyJointDef &joint = pulleys.back();
				joints.push_back(JointTemplate());
				joint.userData = &joints.back();
				CollidableTemplate::ConfigurePulleyJoint(child, joint);
			}
			break;

		case 0xc3b5cf50 /* "mousejoint" */:
			{
				mouses.push_back(b2MouseJointDef());
				b2MouseJointDef &joint = mouses.back();
				joints.push_back(JointTemplate());
				joint.userData = &joints.back();
				CollidableTemplate::ConfigureMouseJoint(child, joint);
			}
			break;

		case 0x19a3586a /* "gearjoint" */:
			{
			}
			break;
		}
	}

	return true;
}


b2World* Collidable::world;

Collidable::Collidable(void)
: id(0), body(NULL) 
{
}

Collidable::Collidable(const CollidableTemplate &aTemplate, unsigned int aId)
: id(aId), body(NULL)
{
}

Collidable::~Collidable(void)
{
}

bool Collidable::SetupJointDef(b2JointDef &joint)
{
	CollidableTemplate::JointTemplate *data = static_cast<CollidableTemplate::JointTemplate *>(joint.userData);
	joint.userData = NULL;
	joint.body1 = Database::collidablebody.Get(data->name1 ? data->name1 : id).Get(data->body1);
	if (!joint.body1)
		return false;
	joint.body2 = Database::collidablebody.Get(data->name2 ? data->name2 : id).Get(data->body2);
	if (!joint.body2)
		return false;
	return true;
}

void Collidable::AddToWorld(void)
{
	const CollidableTemplate &collidable = Database::collidabletemplate.Get(id);

	// for each body...
	Database::Typed<b2Body *> &bodies = Database::collidablebody.Open(id);
	for (Database::Typed<b2BodyDef>::Iterator itor(Database::collidabletemplatebody.Find(id)); itor.IsValid(); ++itor)
	{
		// set body position to entity (HACK)
		b2BodyDef def(itor.GetValue());
		def.userData = reinterpret_cast<void *>(id);
		const Entity *entity = Database::entity.Get(id);
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
		bodies.Put(itor.GetKey(), body);
	}
	Database::collidablebody.Close(id);

	// for each joint
	for (std::list<b2RevoluteJointDef>::const_iterator itor = collidable.revolutes.begin(); itor != collidable.revolutes.end(); ++itor)
	{
		b2RevoluteJointDef joint(*itor);
		if (SetupJointDef(joint))
		{
			joint.anchorPoint = joint.body2->GetWorldPoint(joint.anchorPoint);
			world->CreateJoint(&joint);
		}
	}
	for (std::list<b2PrismaticJointDef>::const_iterator itor = collidable.prismatics.begin(); itor != collidable.prismatics.end(); ++itor)
	{
		b2PrismaticJointDef joint(*itor);
		if (SetupJointDef(joint))
		{
			joint.anchorPoint = joint.body2->GetWorldPoint(joint.anchorPoint);
			world->CreateJoint(&joint);
		}
	}
	for (std::list<b2DistanceJointDef>::const_iterator itor = collidable.distances.begin(); itor != collidable.distances.end(); ++itor)
	{
		b2DistanceJointDef joint(*itor);
		if (SetupJointDef(joint))
		{
			joint.anchorPoint1 = joint.body1->GetWorldPoint(joint.anchorPoint1);
			joint.anchorPoint2 = joint.body2->GetWorldPoint(joint.anchorPoint2);
			world->CreateJoint(&joint);
		}
	}
	for (std::list<b2PulleyJointDef>::const_iterator itor = collidable.pulleys.begin(); itor != collidable.pulleys.end(); ++itor)
	{
		b2PulleyJointDef joint(*itor);
		if (SetupJointDef(joint))
		{
			joint.anchorPoint1 = joint.body1->GetWorldPoint(joint.anchorPoint1);
			joint.anchorPoint2 = joint.body2->GetWorldPoint(joint.anchorPoint2);
			world->CreateJoint(&joint);
		}
	}
	for (std::list<b2MouseJointDef>::const_iterator itor = collidable.mouses.begin(); itor != collidable.mouses.end(); ++itor)
	{
		b2MouseJointDef joint(*itor);
		if (SetupJointDef(joint))
		{
			world->CreateJoint(&joint);
		}
	}
}

void Collidable::RemoveFromWorld(void)
{
	for (Database::Typed<b2Body *>::Iterator itor(Database::collidablebody.Find(id)); itor.IsValid(); ++itor)
	{
		world->DestroyBody(itor.GetValue());
	}
	Database::collidablebody.Delete(id);

	body = NULL;
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
	world->Step(aStep, 16);
	world->m_broadPhase->Validate();

	// for each body...
	for (b2Body* body = world->GetBodyList(); body; body = body->GetNext())
	{
		// if the body is not sleeping or static...
		if (!body->IsSleeping() && !body->IsStatic())
		{
			// get the database key
			Database::Key id = reinterpret_cast<Database::Key>(body->GetUserData());

			// update the entity position (hack)
			Collidable *collidable = Database::collidable.Get(id);
			if (collidable)
			{
				Entity *entity = Database::entity.Get(id);
				if (entity)
				{
					entity->Step();
					entity->SetTransform(body->GetRotation(), Vector2(body->GetOriginPosition()));
					entity->SetVelocity(Vector2(body->GetLinearVelocity()));
				}
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
			Database::Key id1 = reinterpret_cast<Database::Key>(body1->GetUserData());
			Database::Key id2 = reinterpret_cast<Database::Key>(body2->GetUserData());
			float toi = 1.0f;
			for (Database::Typed<Listener>::Iterator itor(Database::collidablelistener.Find(id1)); itor.IsValid(); ++itor)
				itor.GetValue()(id2, toi, c->GetManifolds(), c->GetManifoldCount());
			for (Database::Typed<Listener>::Iterator itor(Database::collidablelistener.Find(id2)); itor.IsValid(); ++itor)
				itor.GetValue()(id1, toi, c->GetManifolds(), c->GetManifoldCount());
		}
	}
}
