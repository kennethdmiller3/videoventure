#include "StdAfx.h"
#include "Collidable.h"
#include "Entity.h"
#include "Link.h"

#ifdef USE_POOL_ALLOCATOR
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

#ifndef COLLIDABLE_SHAPE_DATABASE
static const size_t shapesize =
	std::max(sizeof(b2CircleDef),
#ifdef B2_EDGE_SHAPE_H
	std::max(sizeof(b2PolygonDef),
	sizeof(b2EdgeChainDef))
#else
	sizeof(b2PolygonDef)
#endif
	);
static boost::pool<boost::default_user_allocator_malloc_free> shapepool(shapesize);
#endif

#ifndef COLLIDABLE_JOINT_DATABASE
// joint pool
static const size_t jointsize = 
	std::max(sizeof(b2RevoluteJointDef),
	std::max(sizeof(b2PrismaticJointDef),
	std::max(sizeof(b2DistanceJointDef),
	std::max(sizeof(b2PulleyJointDef),
	sizeof(b2MouseJointDef)))));
static boost::pool<boost::default_user_allocator_malloc_free> jointpool(jointsize);
#endif
#endif


namespace Database
{
	Typed<b2FilterData> collidablefilter(0x5224d988 /* "collidablefilter" */);
	Typed<CollidableTemplate> collidabletemplate(0xa7380c00 /* "collidabletemplate" */);
#ifdef COLLIDABLE_SHAPE_DATABASE
	Typed<Typed<b2CircleDef> > collidabletemplatecircle(0xa72cf124 /* "collidabletemplatecircle" */);
	Typed<Typed<b2PolygonDef> > collidabletemplatepolygon(0x8ce45056 /* "collidabletemplatepolygon" */);
#ifdef B2_EDGE_SHAPE_H
	Typed<Typed<b2EdgeChainDef> > collidabletemplateedgechain(0x24789e46 /* "collidabletemplateedgechain" */);
#endif
#endif
#ifdef COLLIDABLE_JOINT_DATABASE
	Typed<Typed<b2RevoluteJointDef> > collidabletemplaterevolute(0x8c99420e /* "collidabletemplaterevolute" */);
	Typed<Typed<b2PrismaticJointDef> > collidabletemplateprismatic(0xc3d65710 /* "collidabletemplateprismatic" */);
	Typed<Typed<b2DistanceJointDef> > collidabletemplatedistance(0xb17a01db /* "collidabletemplatedistance" */);
	Typed<Typed<b2PulleyJointDef> > collidabletemplatepulley(0x4d2dcd55 /* "collidabletemplatepulley" */);
	Typed<Typed<b2MouseJointDef> > collidabletemplatemouse(0x92dbc29b /* "collidabletemplatemouse" */);
#endif
	Typed<Collidable *> collidable(0x74e9dbae /* "collidable" */);
	Typed<Typed<Collidable::ContactListener> > collidablecontactadd(0x7cf2c45d /* "collidablecontactadd" */);
	Typed<Typed<Collidable::ContactListener> > collidablecontactremove(0x95ed5aba /* "collidablecontactremove" */);
	Typed<Typed<Collidable::BoundaryListener> > collidableboundaryviolation(0x1544aaaf /* "collidableboundaryviolation" */);

	namespace Loader
	{
		class FilterLoader
		{
		public:
			FilterLoader()
			{
				AddConfigure(0x5224d988 /* "collidablefilter" */, Entry(this, &FilterLoader::Configure));

				b2FilterData &filter = Database::collidablefilter.OpenDefault();
				filter = Collidable::GetDefaultFilter();
				Database::collidablefilter.CloseDefault();
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				if (!Database::collidablefilter.Find(aId))
					Database::collidablefilter.Put(aId, Collidable::GetDefaultFilter());
				b2FilterData &filter = Database::collidablefilter.Open(aId);
				ConfigureFilterData(filter, element);
				Database::collidablefilter.Close(aId);
			}
		}
		filterloader;

		class CollidableLoader
		{
		public:
			CollidableLoader()
			{
				AddConfigure(0x74e9dbae /* "collidable" */, Entry(this, &CollidableLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				CollidableTemplate &collidable = Database::collidabletemplate.Open(aId);
				collidable.Configure(element, aId);
				Database::collidabletemplate.Close(aId);
			}
		}
		collidableloader;
	}

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
					Database::collidablecontactadd.Delete(aId);
				}
			}
		}
		collidableinitializer;
	}
}

static void ConfigureFilterCategory(b2FilterData &aFilter, const TiXmlElement *element, const char *name)
{
	int category = 0;
	if (element->QueryIntAttribute(name, &category) == TIXML_SUCCESS)
		aFilter.categoryBits = (category >= 0) ? (1<<category) : 0;
}

static void ConfigureFilterMask(b2FilterData &aFilter, const TiXmlElement *element)
{
	int defvalue = 1;
	if (element->QueryIntAttribute("default", &defvalue) == TIXML_SUCCESS)
		aFilter.maskBits = defvalue ? 0xFFFF : 0x0000;

	char buf[16];
	for (int i = 0; i < 16; i++)
	{
		sprintf(buf, "bit%d", i);
		int bit = 0;
		if (element->QueryIntAttribute(buf, &bit) == TIXML_SUCCESS)
		{
			if (bit)
				aFilter.maskBits |= (1 << i);
			else
				aFilter.maskBits &= ~(1 << i);
		}
	}
}

static void ConfigureFilterGroup(b2FilterData &aFilter, const TiXmlElement *element, const char *name)
{
	int group = aFilter.groupIndex;
	element->QueryIntAttribute(name, &group);
	aFilter.groupIndex = short(group);
}

void ConfigureFilterData(b2FilterData &aFilter, const TiXmlElement *element)
{
	if (const char *name = (Hash(element->Value()) == 0xc7e16877 /* "filter" */) ? element->Attribute("name") : element->Attribute("filter"))
		aFilter = Database::collidablefilter.Get(Hash(name));

	ConfigureFilterCategory(aFilter, element, "category");
	ConfigureFilterMask(aFilter, element);
	ConfigureFilterGroup(aFilter, element, "group");

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch(Hash(child->Value()))
		{
		case 0xcf2f4271 /* "category" */:
			ConfigureFilterCategory(aFilter, child, "value");
			break;

		case 0xe7774569 /* "mask" */:
			ConfigureFilterMask(aFilter, child);
			break;

		case 0x5fb91e8c /* "group" */:
			ConfigureFilterGroup(aFilter, child, "value");
			break;
		}
	}
}

CollidableTemplate::CollidableTemplate(void)
{
}

CollidableTemplate::CollidableTemplate(const CollidableTemplate &aTemplate)
: id(aTemplate.id)
, bodydef(aTemplate.bodydef)
{
#ifndef COLLIDABLE_SHAPE_DATABASE
	// deep-copy the shape list
	for (Database::Typed<b2ShapeDef *>::Iterator itor(&aTemplate.shapes); itor.IsValid(); ++itor)
	{
		const b2ShapeDef *shapesource = itor.GetValue();
		b2ShapeDef *shapedef;
		switch(shapesource->type)
		{
		case e_circleShape:		shapedef = new(shapepool.malloc()) b2CircleDef(*static_cast<const b2CircleDef *>(shapesource)); break;
		case e_polygonShape:	shapedef = new(shapepool.malloc()) b2PolygonDef(*static_cast<const b2PolygonDef *>(shapesource)); break;
#ifdef B2_EDGE_SHAPE_H
		case e_edgeShape:		shapedef = new(shapepool.malloc()) b2EdgeChainDef(*static_cast<const b2EdgeChainDef *>(shapesource)); break;
#endif
		default:				shapedef = NULL; DebugPrint("unsupported shape type %d", shapesource->type); break;
		}
		if (shapedef)
			shapes.Put(itor.GetKey(), shapedef);
	}
#endif

#ifndef COLLIDABLE_JOINT_DATABASE
	// deep-copy the joint list
	// TO DO: update collidable id in each descriptor
	for (Database::Typed<b2JointDef *>::Iterator itor(&aTemplate.joints); itor.IsValid(); ++itor)
	{
		const b2JointDef *jointsource = itor.GetValue();
		b2JointDef *jointdef;
		switch (jointsource->type)
		{
		case e_revoluteJoint:	jointdef = new(jointpool.malloc()) b2RevoluteJointDef(*static_cast<const b2RevoluteJointDef *>(jointsource)); break;
		case e_prismaticJoint:	jointdef = new(jointpool.malloc()) b2PrismaticJointDef(*static_cast<const b2PrismaticJointDef *>(jointsource)); break;
		case e_distanceJoint:	jointdef = new(jointpool.malloc()) b2DistanceJointDef(*static_cast<const b2DistanceJointDef *>(jointsource)); break;
		case e_pulleyJoint:		jointdef = new(jointpool.malloc()) b2PulleyJointDef(*static_cast<const b2PulleyJointDef *>(jointsource)); break;
		case e_mouseJoint:		jointdef = new(jointpool.malloc()) b2MouseJointDef(*static_cast<const b2MouseJointDef *>(jointsource)); break;
		default:				jointdef = NULL; DebugPrint("unsupported joint type %d", jointsource->type); break;
		}
		if (jointdef)
			joints.Put(itor.GetKey(), jointdef);
	}
#endif
}

CollidableTemplate::~CollidableTemplate(void)
{
#ifndef COLLIDABLE_SHAPE_DATABASE
	// free the shape list
	for (Database::Typed<b2ShapeDef *>::Iterator itor(&shapes); itor.IsValid(); ++itor)
	{
		b2ShapeDef *shapedef = itor.GetValue();
		if (shapedef->type == e_edgeShape)
			free(static_cast<b2EdgeChainDef *>(shapedef)->vertices);
		shapedef->~b2ShapeDef();
		shapepool.free(shapedef);
	}
#endif
#ifndef COLLIDABLE_JOINT_DATABASE
	// free the joint list
	for (Database::Typed<b2JointDef *>::Iterator itor(&joints); itor.IsValid(); ++itor)
	{
		b2JointDef *jointdef = itor.GetValue();
		jointdef->~b2JointDef();
		jointpool.free(jointdef);
	}
#endif
}

bool CollidableTemplate::ConfigureShapeItem(const TiXmlElement *element, b2ShapeDef &shape)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xa51be2bb /* "friction" */:
		element->QueryFloatAttribute("value", &shape.friction);
		return true;

	case 0xf59a4f8f /* "restitution" */:
		element->QueryFloatAttribute("value", &shape.restitution);
		return true;

	case 0x72b9059b /* "density" */:
		element->QueryFloatAttribute("value", &shape.density);
		return true;

	case 0xc7e16877 /* "filter" */:
		ConfigureFilterData(shape.filter, element);
		return true;

	case 0xcf2f4271 /* "category" */:
		ConfigureFilterCategory(shape.filter, element, "value");
		return true;

	case 0xe7774569 /* "mask" */:
		ConfigureFilterMask(shape.filter, element);
		return true;

	case 0x5fb91e8c /* "group" */:
		ConfigureFilterGroup(shape.filter, element, "value");
		return true;

	case 0x83b6367b /* "sensor" */:
		{
			int sensor = shape.isSensor;
			element->QueryIntAttribute("value", &sensor);
			shape.isSensor = sensor != 0;
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureShape(const TiXmlElement *element, b2ShapeDef &shape)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureShapeItem(child, shape);
	}
	return true;
}

bool CollidableTemplate::ConfigureCircle(const TiXmlElement *element, b2CircleDef &shape)
{
	element->QueryFloatAttribute("radius", &shape.radius);
	ConfigureShape(element, shape);
	return true;
}

bool CollidableTemplate::ConfigureBox(const TiXmlElement *element, b2PolygonDef &shape)
{
	// half-width and half-height
	float w = 0, h = 0;
	element->QueryFloatAttribute("w", &w);
	element->QueryFloatAttribute("h", &h);

	// center and rotation
	b2Vec2 center(0.0f, 0.0f);
	float rotation = 0.0f;

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x934f4e0a /* "position" */:
			element->QueryFloatAttribute("x", &center.x);
			element->QueryFloatAttribute("y", &center.y);
			if (element->QueryFloatAttribute("angle", &rotation) == TIXML_SUCCESS)
				 rotation *= float(M_PI)/180.0f;
			break;
		}
	}

	shape.SetAsBox(w, h, center, rotation);

	ConfigureShape(element, shape);
	return true;
}

bool CollidableTemplate::ConfigurePolyItem(const TiXmlElement *element, b2PolygonDef &shape)
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
		return ConfigureShapeItem(element, shape);
	}
}

bool CollidableTemplate::ConfigurePoly(const TiXmlElement *element, b2PolygonDef &shape)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigurePolyItem(child, shape);
	}
	return true;
}

#ifdef B2_EDGE_SHAPE_H
bool CollidableTemplate::ConfigureEdgeItem(const TiXmlElement *element, b2EdgeChainDef &shape)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x945367a7 /* "vertex" */:
		shape.vertices = static_cast<b2Vec2 *>(realloc(shape.vertices, (shape.vertexCount + 1) * sizeof(b2Vec2)));
		element->QueryFloatAttribute("x", &shape.vertices[shape.vertexCount].x);
		element->QueryFloatAttribute("y", &shape.vertices[shape.vertexCount].y);
		++shape.vertexCount;
		return true;

	default:
		return ConfigureShapeItem(element, shape);
	}
}

bool CollidableTemplate::ConfigureEdge(const TiXmlElement *element, b2EdgeChainDef &shape)
{
	int isaloop = shape.isALoop;
	element->QueryIntAttribute("loop", &isaloop);
	shape.isALoop = isaloop != 0;

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureEdgeItem(child, shape);
	}
	return true;
}
#endif

bool CollidableTemplate::ConfigureBodyItem(const TiXmlElement *element, b2BodyDef &body)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &body.position.x);
		element->QueryFloatAttribute("y", &body.position.y);
		if (element->QueryFloatAttribute("angle", &body.angle) == TIXML_SUCCESS)
			 body.angle *= float(M_PI)/180.0f;
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

	case 0xa3ae0ca2 /* "startsleep" */:
		{
			int startsleep = body.isSleeping;
			element->QueryIntAttribute("value", &startsleep);
			body.isSleeping = startsleep != 0;
		}
		return true;

	case 0x7a04061b /* "fixedrotation" */:
		{
			int fixedrotation = body.fixedRotation;
			element->QueryIntAttribute("value", &fixedrotation);
			body.fixedRotation = fixedrotation != 0;
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
#ifdef COLLIDABLE_SHAPE_DATABASE
			Database::Typed<b2CircleDef> &shapes = Database::collidabletemplatecircle.Open(id);
			int shapeid = shapes.GetCount()+1;
			b2CircleDef &shape = shapes.Open(shapeid);
			ConfigureCircle(element, shape);
			shapes.Close(shapeid);
			Database::collidabletemplatecircle.Close(id);
#else
			b2CircleDef *shape = new(shapepool.malloc()) b2CircleDef();
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), shape);
			else
				shapes.Put(shapes.GetCount() + 1, shape);
			ConfigureCircle(element, *shape);
#endif
		}
		return true;

	case 0x70c67e32 /* "box" */:
		{
#ifdef COLLIDABLE_SHAPE_DATABASE
			Database::Typed<b2PolygonDef> &shapes = Database::collidabletemplatepolygon.Open(id);
			int shapeid = shapes.GetCount()+1;
			b2PolygonDef &shape = shapes.Open(shapeid);
			ConfigureBox(element, shape);
			Database::collidabletemplatepolygon.Close(id);
#else
			b2PolygonDef *shape = new(shapepool.malloc()) b2PolygonDef();
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), shape);
			else
				shapes.Put(shapes.GetCount() + 1, shape);
			ConfigureBox(element, *shape);
#endif
		}
		return true;

	case 0x84d6a947 /* "poly" */:
		{
#ifdef COLLIDABLE_SHAPE_DATABASE
			Database::Typed<b2PolygonDef> &shapes = Database::collidabletemplatepolygon.Open(id);
			int shapeid = shapes.GetCount()+1;
			b2PolygonDef &shape = shapes.Open(shapeid);
			ConfigurePoly(element, shape);
			Database::collidabletemplatepolygon.Close(id);
#else
			b2PolygonDef *shape = new(shapepool.malloc()) b2PolygonDef();
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), shape);
			else
				shapes.Put(shapes.GetCount() + 1, shape);
			ConfigurePoly(element, *shape);
#endif
		}
		return true;

#ifdef B2_EDGE_SHAPE_H
	case 0x56f6d83c /* "edge" */:
		{
#ifdef COLLIDABLE_SHAPE_DATABASE
			Database::Typed<b2EdgeChainDef> &shapes = Database::collidabletemplateedge.Open(id);
			int shapeid = shapes.GetCount()+1;
			b2EdgeChainDef &shape = shapes.Open(shapeid);
			ConfigureEdge(element, shape);
			Database::collidabletemplateedge.Close(id);
#else
			b2EdgeChainDef *shape = new(shapepool.malloc()) b2EdgeChainDef();
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), shape);
			else
				shapes.Put(shapes.GetCount() + 1, shape);
			ConfigureEdge(element, *shape);
#endif
		}
		return true;
#endif

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureBody(const TiXmlElement *element, b2BodyDef &body)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureBodyItem(child, body);
	}
	return true;
}

bool CollidableTemplate::ConfigureJointItem(const TiXmlElement *element, b2JointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x115ce60c /* "body1" */:
		{
			const char *name = element->Attribute("name");
			joint.body1 = reinterpret_cast<b2Body *>(name ? Hash(name) : 0);
		}
		return true;

	case 0x145ceac5 /* "body2" */:
		{
			const char *name = element->Attribute("name");
			joint.body2 = reinterpret_cast<b2Body *>(name ? Hash(name) : 0);
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

bool CollidableTemplate::ConfigureRevoluteJointItem(const TiXmlElement *element, b2RevoluteJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchor1.x);
		element->QueryFloatAttribute("y", &joint.localAnchor1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchor2.x);
		element->QueryFloatAttribute("y", &joint.localAnchor2.y);
		return true;

	case 0xad544418 /* "angle" */:
		if (element->QueryFloatAttribute("value", &joint.referenceAngle) == TIXML_SUCCESS)
			joint.referenceAngle *= float(M_PI)/180.0f;
		return true;

	case 0x32dad934 /* "limit" */:
		if (element->QueryFloatAttribute("lower", &joint.lowerAngle) == TIXML_SUCCESS)
			joint.lowerAngle *= float(M_PI) / 180.0f;
		if (element->QueryFloatAttribute("upper", &joint.upperAngle) == TIXML_SUCCESS)
			joint.upperAngle *= float(M_PI) / 180.0f;
		joint.enableLimit = true;
		return true;

	case 0xcaf08472 /* "motor" */:
		element->QueryFloatAttribute("torque", &joint.maxMotorTorque);
		if (element->QueryFloatAttribute("speed", &joint.motorSpeed) == TIXML_SUCCESS)
			joint.motorSpeed *= float(M_PI) / 180.0f;
		joint.enableMotor = true;
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigureRevoluteJoint(const TiXmlElement *element, b2RevoluteJointDef &joint)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureRevoluteJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ConfigurePrismaticJointItem(const TiXmlElement *element, b2PrismaticJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchor1.x);
		element->QueryFloatAttribute("y", &joint.localAnchor1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchor2.x);
		element->QueryFloatAttribute("y", &joint.localAnchor2.y);
		return true;

	case 0x6d2badf4 /* "axis" */:
		element->QueryFloatAttribute("x", &joint.localAxis1.x);
		element->QueryFloatAttribute("y", &joint.localAxis1.y);
		return true;

	case 0xad544418 /* "angle" */:
		if (element->QueryFloatAttribute("value", &joint.referenceAngle) == TIXML_SUCCESS)
			joint.referenceAngle *= float(M_PI)/180.0f;
		return true;

	case 0x32dad934 /* "limit" */:
		element->QueryFloatAttribute("lower", &joint.lowerTranslation);
		element->QueryFloatAttribute("upper", &joint.upperTranslation);
		joint.enableLimit = true;
		return true;

	case 0xcaf08472 /* "motor" */:
		element->QueryFloatAttribute("force", &joint.maxMotorForce);
		element->QueryFloatAttribute("speed", &joint.motorSpeed);
		joint.enableMotor = true;
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigurePrismaticJoint(const TiXmlElement *element, b2PrismaticJointDef &joint)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigurePrismaticJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ConfigureDistanceJointItem(const TiXmlElement *element, b2DistanceJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchor1.x);
		element->QueryFloatAttribute("y", &joint.localAnchor1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchor2.x);
		element->QueryFloatAttribute("y", &joint.localAnchor2.y);
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigureDistanceJoint(const TiXmlElement *element, b2DistanceJointDef &joint)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureDistanceJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ConfigurePulleyJointItem(const TiXmlElement *element, b2PulleyJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe1acc15d /* "ground1" */:
		element->QueryFloatAttribute("x", &joint.groundAnchor1.x);
		element->QueryFloatAttribute("y", &joint.groundAnchor1.y);
		return true;

	case 0xdeacbca4 /* "ground2" */:
		element->QueryFloatAttribute("x", &joint.groundAnchor2.x);
		element->QueryFloatAttribute("y", &joint.groundAnchor2.y);
		return true;

	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchor1.x);
		element->QueryFloatAttribute("y", &joint.localAnchor1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchor2.x);
		element->QueryFloatAttribute("y", &joint.localAnchor2.y);
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
		return ConfigureJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigurePulleyJoint(const TiXmlElement *element, b2PulleyJointDef &joint)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigurePulleyJointItem(child, joint);
	}
	return true;
}

bool CollidableTemplate::ConfigureMouseJointItem(const TiXmlElement *element, b2MouseJointDef &joint)
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
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

bool CollidableTemplate::ConfigureMouseJoint(const TiXmlElement *element, b2MouseJointDef &joint)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureMouseJointItem(child, joint);
	}
	return true;
}


bool CollidableTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	// save identifier
	this->id = id;

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *name = child->Value();
		switch (Hash(name))
		{
		case 0xdbaa7975 /* "body" */:
			{
				// set up the collidable body
				CollidableTemplate::ConfigureBody(child, bodydef);
			}
			break;

		case 0xef2f9539 /* "revolutejoint" */:
			{
#ifdef COLLIDABLE_JOINT_DATABASE
				Database::Typed<b2RevoluteJointDef> &joints = Database::collidabletemplaterevolute.Open(id);
				unsigned int jointid = joints.GetCount()+1;
				b2RevoluteJointDef &joint = joints.Open(jointid);
				CollidableTemplate::ConfigureRevoluteJoint(child, joint);
				joints.Close(jointid);
				Database::collidabletemplaterevolute.Close(id);
#else
				b2RevoluteJointDef *joint = new(jointpool.malloc()) b2RevoluteJointDef();
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), joint);
				else
					joints.Put(joints.GetCount() + 1, joint);
				CollidableTemplate::ConfigureRevoluteJoint(child, *joint);
#endif
			}
			break;

		case 0x4954853d /* "prismaticjoint" */:
			{
#ifdef COLLIDABLE_JOINT_DATABASE
				Database::Typed<b2PrismaticJointDef> &joints = Database::collidabletemplateprismatic.Open(id);
				unsigned int jointid = joints.GetCount()+1;
				b2PrismaticJointDef &joint = joints.Open(jointid);
				CollidableTemplate::ConfigurePrismaticJoint(child, joint);
				joints.Close(jointid);
				Database::collidabletemplaterevolute.Close(id);
#else
				b2PrismaticJointDef *joint = new(jointpool.malloc()) b2PrismaticJointDef();
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), joint);
				else
					joints.Put(joints.GetCount() + 1, joint);
				CollidableTemplate::ConfigurePrismaticJoint(child, *joint);
#endif
			}
			break;

		case 0x6932d1ee /* "distancejoint" */:
			{
#ifdef COLLIDABLE_JOINT_DATABASE
				Database::Typed<b2DistanceJointDef> &joints = Database::collidabletemplatedistance.Open(id);
				unsigned int jointid = joints.GetCount()+1;
				b2DistanceJointDef &joint = joints.Open(jointid);
				CollidableTemplate::ConfigureDistanceJoint(child, joint);
				joints.Close(jointid);
				Database::collidabletemplaterevolute.Close(id);
#else
				b2DistanceJointDef *joint = new(jointpool.malloc()) b2DistanceJointDef();
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), joint);
				else
					joints.Put(joints.GetCount() + 1, joint);
				CollidableTemplate::ConfigureDistanceJoint(child, *joint);
#endif
			}
			break;

		case 0xdd003dc4 /* "pulleyjoint" */:
			{
#ifdef COLLIDABLE_JOINT_DATABASE
				Database::Typed<b2PulleyJointDef> &joints = Database::collidabletemplatepulley.Open(id);
				unsigned int jointid = joints.GetCount()+1;
				b2PulleyJointDef &joint = joints.Open(jointid);
				CollidableTemplate::ConfigurePulleyJoint(child, joint);
				joints.Close(jointid);
				Database::collidabletemplaterevolute.Close(id);
#else
				b2PulleyJointDef *joint = new(jointpool.malloc()) b2PulleyJointDef();
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), joint);
				else
					joints.Put(joints.GetCount() + 1, joint);
				CollidableTemplate::ConfigurePulleyJoint(child, *joint);
#endif
			}
			break;

		case 0xc3b5cf50 /* "mousejoint" */:
			{
#ifdef COLLIDABLE_JOINT_DATABASE
				Database::Typed<b2MouseJointDef> &joints = Database::collidabletemplatemouse.Open(id);
				unsigned int jointid = joints.GetCount()+1;
				b2MouseJointDef &joint = joints.Open(jointid);
				CollidableTemplate::ConfigureMouseJoint(child, joint);
				joints.Close(jointid);
				Database::collidabletemplaterevolute.Close(id);
#else
				b2MouseJointDef *joint = new(jointpool.malloc()) b2MouseJointDef();
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), joint);
				else
					joints.Put(joints.GetCount() + 1, joint);
				CollidableTemplate::ConfigureMouseJoint(child, *joint);
#endif
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

bool CollidableTemplate::SetupLinkJoint(const LinkTemplate &linktemplate, unsigned int aId, unsigned int aSecondary)
{
#ifdef COLLIDABLE_JOINT_DATABASE
	// add a revolute joint to the linked template (HACK)
	Database::Typed<b2RevoluteJointDef> &joints = Database::collidabletemplaterevolute.Open(aSecondary);
	unsigned int jointid = joints.GetCount()+1;
	b2RevoluteJointDef &joint = joints.Open(jointid);

	// configure the joint definition
	joint.body1 = reinterpret_cast<b2Body *>(aId);
	joint.body2 = reinterpret_cast<b2Body *>(aSecondary);
	joint.localAnchor1.Set(linktemplate.mOffset.p.x, linktemplate.mOffset.p.y);
	joint.localAnchor2.Set(0, 0);
	joint.referenceAngle = linktemplate.mOffset.Angle();
	if (linktemplate.mUpdateAngle)
	{
		joint.lowerAngle = 0.0f;
		joint.upperAngle = 0.0f;
		joint.enableLimit = true;
	}

	//
	joints.Close(jointid);
	Database::collidabletemplaterevolute.Close(id);
#else
	// add a revolute joint to the linked template (HACK)
	b2RevoluteJointDef *joint = new(jointpool.malloc()) b2RevoluteJointDef();
	joints.Put(aSecondary, joint);
	
	// configure the joint definition
	joint->body1 = reinterpret_cast<b2Body *>(aId);
	joint->body2 = reinterpret_cast<b2Body *>(aSecondary);
	joint->localAnchor1.Set(linktemplate.mOffset.p.x, linktemplate.mOffset.p.y);
	joint->localAnchor2.Set(0, 0);
	joint->referenceAngle = linktemplate.mOffset.Angle();
	if (linktemplate.mUpdateAngle)
	{
		joint->lowerAngle = 0.0f;
		joint->upperAngle = 0.0f;
		joint->enableLimit = true;
	}
#endif
	return true;
}


b2World* Collidable::world;
b2AABB Collidable::boundary;

/// Implement this class to get collision results. You can use these results for
/// things like sounds and game logic. You can also use this class to tweak contact 
/// settings. These tweaks persist until you tweak the settings again or the contact
/// is destroyed.
/// @warning You cannot create/destroy Box2D entities inside these callbacks.
class ContactListener : public b2ContactListener
{
public:

	/// Called when a contact point is added. This includes the geometry
	/// and the forces.
	virtual void Add(const b2ContactPoint* point)
	{
		b2Shape *shape1 = point->shape1;
		b2Shape *shape2 = point->shape2;
		Database::Key id1 = reinterpret_cast<Database::Key>(shape1->GetUserData());
		Database::Key id2 = reinterpret_cast<Database::Key>(shape2->GetUserData());
		for (Database::Typed<Collidable::ContactListener>::Iterator itor(Database::collidablecontactadd.Find(id1)); itor.IsValid(); ++itor)
			itor.GetValue()(id1, id2, 0.0f /*shape1->GetBody()->m_sweep.t0*/, *point);
		for (Database::Typed<Collidable::ContactListener>::Iterator itor(Database::collidablecontactadd.Find(id2)); itor.IsValid(); ++itor)
			itor.GetValue()(id2, id1, 0.0f /*shape2->GetBody()->m_sweep.t0*/, *point);
	};

	/// Called when a contact point persists. This includes the geometry
	/// and the forces.
	virtual void Persist(const b2ContactPoint* point)
	{
	}

	/// Called when a contact point is removed. This includes the last
	/// computed geometry and forces.
	virtual void Remove(const b2ContactPoint* point)
	{
		b2Shape *shape1 = point->shape1;
		b2Shape *shape2 = point->shape2;
		Database::Key id1 = reinterpret_cast<Database::Key>(shape1->GetUserData());
		Database::Key id2 = reinterpret_cast<Database::Key>(shape2->GetUserData());
		for (Database::Typed<Collidable::ContactListener>::Iterator itor(Database::collidablecontactremove.Find(id1)); itor.IsValid(); ++itor)
			itor.GetValue()(id1, id2, 0.0f /*shape1->GetBody()->m_sweep.t0*/, *point);
		for (Database::Typed<Collidable::ContactListener>::Iterator itor(Database::collidablecontactremove.Find(id2)); itor.IsValid(); ++itor)
			itor.GetValue()(id2, id1, 0.0f /*shape2->GetBody()->m_sweep.t0*/, *point);
	}

	// Called after a contact is resolved (?)
	virtual void Result(const b2ContactResult* result)
	{
	}
}
contactListener;

/// This is called when a body's shape passes outside of the world boundary.
class BoundaryListener : public b2BoundaryListener, public Updatable
{
public:
	BoundaryListener(void)
		: Updatable(0x725d4407 /* "boundarylistener" */)
	{
		SetAction(Updatable::Action(this, &BoundaryListener::Update));
	}
	virtual ~BoundaryListener() {}

	/// This is called for each body that leaves the world boundary.
	/// @warning you can't modify the world inside this callback.
	virtual void Violation(b2Body* body)
	{
		Database::Key id = reinterpret_cast<Database::Key>(body->GetUserData());
		for (Database::Typed<Collidable::BoundaryListener>::Iterator itor(Database::collidableboundaryviolation.Find(id)); itor.IsValid(); ++itor)
			itor.GetValue()(id);
		if (mList.empty())
			Activate();
		mList.push_back(id);
	}

	void Update(float aStep)
	{
		for (std::vector<Database::Key>::iterator itor = mList.begin(); itor != mList.end(); ++itor)
			Database::Delete(*itor);
		mList.clear();
		Deactivate();
	}

private:
	std::vector<Database::Key> mList;
}
boundaryListener;




#ifdef COLLIDABLE_DEBUG_DRAW
// This class implements debug drawing callbacks that are invoked
// inside b2World::Step.
class DebugDraw : public b2DebugDraw
{
public:
	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
	void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
	void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color);
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
	void DrawAxis(const b2Vec2& point, const b2Vec2& axis, const b2Color& color);
	void DrawXForm(const b2XForm& xf);
	void DrawForce(const b2Vec2& point, const b2Vec2& force, const b2Color& color);
	void DrawAABB(b2AABB* aabb, const b2Color& c);
}
debugDraw;

void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	glColor3f(color.r, color.g, color.b);
	glBegin(GL_LINE_LOOP);
	for (int32 i = 0; i < vertexCount; ++i)
	{
		glVertex2f(vertices[i].x, vertices[i].y);
	}
	glEnd();
}

void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	glColor4f(0.5f * color.r, 0.5f * color.g, 0.5f * color.b, 0.5f);
	glBegin(GL_TRIANGLE_FAN);
	for (int32 i = 0; i < vertexCount; ++i)
	{
		glVertex2f(vertices[i].x, vertices[i].y);
	}
	glEnd();

	glColor4f(color.r, color.g, color.b, 1.0f);
	glBegin(GL_LINE_LOOP);
	for (int32 i = 0; i < vertexCount; ++i)
	{
		glVertex2f(vertices[i].x, vertices[i].y);
	}
	glEnd();
}

void DebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
	const float32 k_segments = 16.0f;
	const float32 k_increment = 2.0f * b2_pi / k_segments;
	float32 theta = 0.0f;
	glColor3f(color.r, color.g, color.b);
	glBegin(GL_LINE_LOOP);
	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
		glVertex2f(v.x, v.y);
		theta += k_increment;
	}
	glEnd();
}

void DebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
	const float32 k_segments = 16.0f;
	const float32 k_increment = 2.0f * b2_pi / k_segments;
	float32 theta = 0.0f;
	glColor4f(0.5f * color.r, 0.5f * color.g, 0.5f * color.b, 0.5f);
	glBegin(GL_TRIANGLE_FAN);
	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
		glVertex2f(v.x, v.y);
		theta += k_increment;
	}
	glEnd();

	theta = 0.0f;
	glColor4f(color.r, color.g, color.b, 1.0f);
	glBegin(GL_LINE_LOOP);
	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
		glVertex2f(v.x, v.y);
		theta += k_increment;
	}
	glEnd();

	b2Vec2 p = center + radius * axis;
	glBegin(GL_LINES);
	glVertex2f(center.x, center.y);
	glVertex2f(p.x, p.y);
	glEnd();
}

void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	glColor3f(color.r, color.g, color.b);
	glBegin(GL_LINES);
	glVertex2f(p1.x, p1.y);
	glVertex2f(p2.x, p2.y);
	glEnd();
}

void DebugDraw::DrawXForm(const b2XForm& xf)
{
	b2Vec2 p1 = xf.position, p2;
	const float32 k_axisScale = 0.4f;
	glBegin(GL_LINES);
	
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(p1.x, p1.y);
	p2 = p1 + k_axisScale * xf.R.col1;
	glVertex2f(p2.x, p2.y);

	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex2f(p1.x, p1.y);
	p2 = p1 + k_axisScale * xf.R.col2;
	glVertex2f(p2.x, p2.y);

	glEnd();
}

void DebugDraw::DrawPoint(const b2Vec2& p, float32 size, const b2Color& color)
{
	glPointSize(size);
	glBegin(GL_POINTS);
	glColor3f(color.r, color.g, color.b);
	glVertex2f(p.x, p.y);
	glEnd();
	glPointSize(1.0f);
}

void DebugDraw::DrawAABB(b2AABB* aabb, const b2Color& c)
{
	glColor3f(c.r, c.g, c.b);
	glBegin(GL_LINE_LOOP);
	glVertex2f(aabb->lowerBound.x, aabb->lowerBound.y);
	glVertex2f(aabb->upperBound.x, aabb->lowerBound.y);
	glVertex2f(aabb->upperBound.x, aabb->upperBound.y);
	glVertex2f(aabb->lowerBound.x, aabb->upperBound.y);
	glEnd();
}

void DebugDraw::DrawForce(const b2Vec2& point, const b2Vec2& force, const b2Color& color)
{
	const float32 k_forceScale = 0.1f;
	b2Vec2 p1 = point;
	b2Vec2 p2 = point + k_forceScale * force;
	DrawSegment(p1, p2, color);
}
#endif


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

bool Collidable::CreateJoint(const b2JointDef &jointtemplate) const
{
	b2JointDef &joint = *reinterpret_cast<b2JointDef *>(_alloca(jointsize));
	memcpy(&joint, &jointtemplate, jointsize);
	unsigned int id1 = reinterpret_cast<unsigned int>(joint.body1);
	joint.userData = NULL;
	Collidable *coll1 = Database::collidable.Get(id1 ? id1 : id);
	if (!coll1)
		return false;
	joint.body1 = coll1->GetBody();
	if (!joint.body1)
		return false;
	unsigned int id2 = reinterpret_cast<unsigned int>(joint.body2);
	Collidable *coll2 = Database::collidable.Get(id2 ? id2 : id);
	if (!coll2)
		return false;
	joint.body2 = coll2->GetBody();
	if (!joint.body2)
		return false;
	world->CreateJoint(&joint);
	return true;
}

void Collidable::AddToWorld(void)
{
	const CollidableTemplate &collidable = Database::collidabletemplate.Get(id);

	// copy the body definition
	b2BodyDef def(collidable.bodydef);

	// set userdata identifier and body position to entity (HACK)
	def.userData = reinterpret_cast<void *>(id);
	const Entity *entity = Database::entity.Get(id);
	if (entity)
	{
		def.angle = entity->GetAngle();
		def.position = entity->GetPosition();
	}

	// create the body
	body = world->CreateBody(&def);

	// add shapes
#ifdef COLLIDABLE_SHAPE_DATABASE
	const Database::Typed<b2CircleDef> &circles = Database::collidabletemplatecircle.Get(id);
	for (Database::Typed<b2CircleDef>::Iterator circleitor(&circles); circleitor.IsValid(); ++circleitor)
	{
		b2CircleDef circle(circleitor.GetValue());
		circle.userData = reinterpret_cast<void *>(id);
		body->CreateShape(&circle);
	}
	const Database::Typed<b2PolygonDef> &polygons = Database::collidabletemplatepolygon.Get(id);
	for (Database::Typed<b2PolygonDef>::Iterator polygonitor(&polygons); polygonitor.IsValid(); ++polygonitor)
	{
		b2PolygonDef polygon(polygonitor.GetValue());
		polygon.userData = reinterpret_cast<void *>(id);
		body->CreateShape(&polygon);
	}
#else
	for (Database::Typed<b2ShapeDef *>::Iterator itor(&collidable.shapes); itor.IsValid(); ++itor)
	{
		b2ShapeDef &shapedef = *reinterpret_cast<b2ShapeDef *>(_alloca(shapesize));
		memcpy(&shapedef, itor.GetValue(), shapesize);
		shapedef.userData = reinterpret_cast<void *>(id);
		body->CreateShape(&shapedef);
	}
#endif

	// compute mass
	body->SetMassFromShapes();

	if (entity)
	{
		body->SetLinearVelocity(entity->GetVelocity());
		body->SetAngularVelocity(entity->GetOmega());
	}

#ifdef COLLIDABLE_JOINT_DATABASE
	// for each joint...
	const Database::Typed<b2RevoluteJointDef> &revolutes = Database::collidabletemplaterevolute.Get(id);
	for (Database::Typed<b2RevoluteJointDef>::Iterator revoluteitor(&revolutes); revoluteitor.IsValid(); ++revoluteitor)
		CreateJoint(revoluteitor.GetValue());
	const Database::Typed<b2PrismaticJointDef> &prismatics = Database::collidabletemplateprismatic.Get(id);
	for (Database::Typed<b2PrismaticJointDef>::Iterator prismaticitor(&prismatics); prismaticitor.IsValid(); ++prismaticitor)
		CreateJoint(prismaticitor.GetValue());
	const Database::Typed<b2DistanceJointDef> &distances = Database::collidabletemplatedistance.Get(id);
	for (Database::Typed<b2DistanceJointDef>::Iterator distanceitor(&distances); distanceitor.IsValid(); ++distanceitor)
		CreateJoint(distanceitor.GetValue());
	const Database::Typed<b2PulleyJointDef> &pulleys = Database::collidabletemplatepulley.Get(id);
	for (Database::Typed<b2PulleyJointDef>::Iterator pulleyitor(&pulleys); pulleyitor.IsValid(); ++pulleyitor)
		CreateJoint(pulleyitor.GetValue());
	const Database::Typed<b2MouseJointDef> &mouses = Database::collidabletemplatemouse.Get(id);
	for (Database::Typed<b2MouseJointDef>::Iterator mouseitor(&mouses); mouseitor.IsValid(); ++mouseitor)
		CreateJoint(mouseitor.GetValue());
#else
	// for each joint...
	for (Database::Typed<b2JointDef *>::Iterator itor(&collidable.joints); itor.IsValid(); ++itor)
		CreateJoint(*itor.GetValue());
#endif
}

void Collidable::RemoveFromWorld(void)
{
	world->DestroyBody(body);
	body = NULL;
}



// create collision world
void Collidable::WorldInit(float aMinX, float aMinY, float aMaxX, float aMaxY, bool aWall)
{
	// save boundary extents
	boundary.lowerBound.Set(aMinX, aMinY);
	boundary.upperBound.Set(aMaxX, aMaxY);

	// create physics world
	b2AABB aabb;
#ifdef B2_EDGE_SHAPE_H
	aabb.lowerBound.Set(aMinX, aMinY);
	aabb.upperBound.Set(aMaxX, aMaxY);
#else
	aabb.lowerBound.Set(aMinX - 32, aMinY - 32);
	aabb.upperBound.Set(aMaxX + 32, aMaxY + 32);
#endif
	b2Vec2 gravity;
	gravity.Set(0.0f, 0.0f);
	bool doSleep = true;
	world = new b2World(aabb, gravity, doSleep);

	// set contact listener
	world->SetContactListener(&contactListener);

	// set boundary listener
	world->SetBoundaryListener(&boundaryListener);

#ifdef COLLIDABLE_DEBUG_DRAW
	// set debug render
	world->SetDebugDraw(&debugDraw);
	debugDraw.SetFlags(~0U);
#endif

	if (aWall)
	{
		// create perimeter wall
		b2Body *body = world->GetGroundBody();

#ifdef B2_EDGE_SHAPE_H
		b2Vec2 vertex[4] =
		{
			b2Vec2(aMaxX * (1.0f - FLT_EPSILON), aMaxY * (1.0f - FLT_EPSILON)),
			b2Vec2(aMaxX * (1.0f - FLT_EPSILON), aMinY * (1.0f - FLT_EPSILON)),
			b2Vec2(aMinX * (1.0f - FLT_EPSILON), aMinY * (1.0f - FLT_EPSILON)),
			b2Vec2(aMinX * (1.0f - FLT_EPSILON), aMaxY * (1.0f - FLT_EPSILON))
		};

		b2EdgeChainDef edge;
		edge.vertexCount = SDL_arraysize(vertex);
		edge.vertices = vertex;
		edge.isALoop = true;
		body->CreateShape(&edge);
#else
		b2PolygonDef top;
		top.SetAsBox(0.5f * (aMaxX - aMinX) + 32, 16, b2Vec2(0.5f * (aMaxX + aMinX), aMinY - 16), 0);
		body->CreateShape(&top);

		b2PolygonDef bottom;
		bottom.SetAsBox(0.5f * (aMaxX - aMinX) + 32, 16, b2Vec2(0.5f * (aMaxX + aMinX), aMaxY + 16), 0);
		body->CreateShape(&bottom);

		b2PolygonDef left;
		left.SetAsBox(16, 0.5f * (aMaxY - aMinY) + 32, b2Vec2(aMinX - 16, 0.5f * (aMaxY + aMinY)), 0);
		body->CreateShape(&left);

		b2PolygonDef right;
		right.SetAsBox(16, 0.5f * (aMaxY - aMinY) + 32, b2Vec2(aMaxX + 16, 0.5f * (aMaxY + aMinY)), 0);
		body->CreateShape(&right);
#endif
	}
}

void Collidable::WorldDone(void)
{
	delete world;
	world = NULL;
}

void Collidable::CollideAll(float aStep)
{
	// exit if no world
	if (!world)
		return;

	// step the physics world
	world->Step(aStep, 16, 16);

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
					entity->SetTransform(body->GetAngle(), Vector2(body->GetPosition()));
					entity->SetVelocity(Vector2(body->GetLinearVelocity()));
					entity->SetOmega(body->GetAngularVelocity());
				}
			}
		}
	}
}

unsigned int Collidable::TestSegment(const b2Segment &aSegment, const b2FilterData &aFilter, unsigned int aId,
									 float &aLambda, b2Vec2 &aNormal, b2Shape *&aShape)
{
	// get nearby shapes
	b2AABB aabb;
	aabb.lowerBound.Set(std::min(aSegment.p1.x, aSegment.p2.x), std::min(aSegment.p1.y, aSegment.p2.y));
	aabb.upperBound.Set(std::max(aSegment.p1.x, aSegment.p2.x), std::max(aSegment.p1.y, aSegment.p2.y));
	b2Shape* shapes[b2_maxProxies];
	int32 count = world->Query(aabb, shapes, b2_maxProxies);

	// hit anything?
	unsigned hitId = 0;

	// for each shape...
	for (int32 i = 0; i < count; ++i)
	{
		// get the shape
		b2Shape *shape = shapes[i];

		// skip unhittable shapes
		if (shape->IsSensor())
			continue;
		if (!CheckFilter(aFilter, shape->GetFilterData()))
			continue;

		// get the parent body
		b2Body* body = shape->GetBody();

		// get the collidable identifier
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip self
		if (targetId == aId)
			continue;

		// if the segment intersects the shape...
		if (shape->TestSegment(body->GetXForm(), &aLambda, &aNormal, aSegment, aLambda) != 0)
		{
			// update hit shape
			aShape = shape;

			// update hit entity identifier
			hitId = reinterpret_cast<unsigned int>(body->GetUserData());
		}
	}

	return hitId;
}
