#include "StdAfx.h"
#include "Collidable.h"
#include "Entity.h"
#include "Link.h"

namespace std
{
	struct RTTI_NOT_SUPPORTED;
	typedef RTTI_NOT_SUPPORTED type_info;
}
#define typeid *( ::std::type_info* )sizeof 
#include <boost/variant.hpp>


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
#endif

struct CollisionCircleDef 
{
	b2FixtureDef mFixture;
	b2CircleShape mShape;

	CollisionCircleDef()
	{
		mFixture.shape = &mShape;
	}
	CollisionCircleDef(const CollisionCircleDef &aSrc)
	{
		mFixture = aSrc.mFixture;
		mShape = aSrc.mShape;
		mFixture.shape = &mShape;
	}
};
struct CollisionPolygonDef 
{
	b2FixtureDef mFixture;
	b2PolygonShape mShape;

	CollisionPolygonDef()
	{
		mFixture.shape = &mShape;
	}
	CollisionPolygonDef(const CollisionPolygonDef &aSrc)
	{
		mFixture = aSrc.mFixture;
		mShape = aSrc.mShape;
		mFixture.shape = &mShape;
	}
};

typedef boost::variant<int, CollisionCircleDef, CollisionPolygonDef> CollisionShapeDef;
typedef boost::variant<int, b2RevoluteJointDef, b2PrismaticJointDef, b2DistanceJointDef, b2PulleyJointDef, b2LineJointDef> CollisionJointDef;

namespace Database
{
	Typed<b2Filter> collidablefilter(0x5224d988 /* "collidablefilter" */);
	Typed<CollidableTemplate> collidabletemplate(0xa7380c00 /* "collidabletemplate" */);
	Typed<Typed<CollisionShapeDef> > collidableshapes(0x08366028 /* "collidableshapes" */);
	Typed<Typed<CollisionJointDef> > collidablejoints(0x9c0ba7db /* "collidablejoints" */);
	Typed<Collidable *> collidable(0x74e9dbae /* "collidable" */);
	Typed<Collidable::ContactSignal> collidablecontactadd(0x7cf2c45d /* "collidablecontactadd" */);
	Typed<Collidable::ContactSignal> collidablecontactremove(0x95ed5aba /* "collidablecontactremove" */);

	namespace Loader
	{
		class FilterLoader
		{
		public:
			FilterLoader()
			{
				AddConfigure(0x5224d988 /* "collidablefilter" */, Entry(this, &FilterLoader::Configure));

				b2Filter &filter = Database::collidablefilter.OpenDefault();
				filter = Collidable::GetDefaultFilter();
				Database::collidablefilter.CloseDefault();
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				if (!Database::collidablefilter.Find(aId))
					Database::collidablefilter.Put(aId, Collidable::GetDefaultFilter());
				b2Filter &filter = Database::collidablefilter.Open(aId);
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

static void ConfigureFilterCategory(b2Filter &aFilter, const TiXmlElement *element, const char *name)
{
	int category = 0;
	if (element->QueryIntAttribute(name, &category) == TIXML_SUCCESS)
		aFilter.categoryBits = (category >= 0) ? (1<<category) : 0;
}

static void ConfigureFilterMask(b2Filter &aFilter, const TiXmlElement *element)
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

static void ConfigureFilterGroup(b2Filter &aFilter, const TiXmlElement *element, const char *name)
{
	int group = aFilter.groupIndex;
	element->QueryIntAttribute(name, &group);
	aFilter.groupIndex = short(group);
}

void ConfigureFilterData(b2Filter &aFilter, const TiXmlElement *element)
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
: id(0)
{
}

CollidableTemplate::CollidableTemplate(const CollidableTemplate &aTemplate)
: id(aTemplate.id)
, bodydef(aTemplate.bodydef)
{
}

CollidableTemplate::~CollidableTemplate(void)
{
}

bool CollidableTemplate::ConfigureFixtureItem(const TiXmlElement *element, b2FixtureDef &fixture)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xa51be2bb /* "friction" */:
		element->QueryFloatAttribute("value", &fixture.friction);
		return true;

	case 0xf59a4f8f /* "restitution" */:
		element->QueryFloatAttribute("value", &fixture.restitution);
		return true;

	case 0x72b9059b /* "density" */:
		element->QueryFloatAttribute("value", &fixture.density);
		return true;

	case 0xc7e16877 /* "filter" */:
		ConfigureFilterData(fixture.filter, element);
		return true;

	case 0xcf2f4271 /* "category" */:
		ConfigureFilterCategory(fixture.filter, element, "value");
		return true;

	case 0xe7774569 /* "mask" */:
		ConfigureFilterMask(fixture.filter, element);
		return true;

	case 0x5fb91e8c /* "group" */:
		ConfigureFilterGroup(fixture.filter, element, "value");
		return true;

	case 0x83b6367b /* "sensor" */:
		{
			int sensor = fixture.isSensor;
			element->QueryIntAttribute("value", &sensor);
			fixture.isSensor = sensor != 0;
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureFixture(const TiXmlElement *element, b2FixtureDef &fixture)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureFixtureItem(child, fixture);
	}
	return true;
}

bool CollidableTemplate::ConfigureCircle(const TiXmlElement *element, b2CircleShape &shape)
{
	element->QueryFloatAttribute("radius", &shape.m_radius);
	return true;
}

bool CollidableTemplate::ConfigureBox(const TiXmlElement *element, b2PolygonShape &shape)
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
	return true;
}

bool CollidableTemplate::ConfigurePolyItem(const TiXmlElement *element, b2PolygonShape &shape)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x945367a7 /* "vertex" */:
		element->QueryFloatAttribute("x", &shape.m_vertices[shape.m_vertexCount].x);
		element->QueryFloatAttribute("y", &shape.m_vertices[shape.m_vertexCount].y);
		++shape.m_vertexCount;
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigurePoly(const TiXmlElement *element, b2PolygonShape &shape)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigurePolyItem(child, shape);
	}

	// update other properties
	shape.Set(shape.m_vertices, shape.m_vertexCount);

	return true;
}

bool CollidableTemplate::ConfigureEdge(const TiXmlElement *element, b2PolygonShape &shape)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *name = child->Value();
		switch (Hash(name))
		{
		case 0x154c1122 /* "vertex1" */:
			child->QueryFloatAttribute("x", &shape.m_vertices[0].x);
			child->QueryFloatAttribute("y", &shape.m_vertices[0].y);
			break;

		case 0x144c0f8f /* "vertex2" */:
			child->QueryFloatAttribute("x", &shape.m_vertices[1].x);
			child->QueryFloatAttribute("y", &shape.m_vertices[1].y);
			break;
		}
	}

	shape.SetAsEdge(shape.m_vertices[0], shape.m_vertices[1]);

	return true;
}

bool CollidableTemplate::ConfigureBodyItem(const TiXmlElement *element, b2BodyDef &body, unsigned int id)
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
			CollisionCircleDef def;
			ConfigureFixture(element, def.mFixture);
			ConfigureCircle(element, def.mShape);
			Database::Typed<CollisionShapeDef> &shapes = Database::collidableshapes.Open(id);
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), def);
			else
				shapes.Put(shapes.GetCount() + 1, def);
			Database::collidableshapes.Close(id);
		}
		return true;

	case 0x70c67e32 /* "box" */:
		{
			CollisionPolygonDef def;
			ConfigureFixture(element, def.mFixture);
			ConfigureBox(element, def.mShape);
			Database::Typed<CollisionShapeDef> &shapes = Database::collidableshapes.Open(id);
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), def);
			else
				shapes.Put(shapes.GetCount() + 1, def);
			Database::collidableshapes.Close(id);
		}
		return true;

	case 0x84d6a947 /* "poly" */:
		{
			CollisionPolygonDef def;
			ConfigureFixture(element, def.mFixture);
			ConfigurePoly(element, def.mShape);
			Database::Typed<CollisionShapeDef> &shapes = Database::collidableshapes.Open(id);
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), def);
			else
				shapes.Put(shapes.GetCount() + 1, def);
			Database::collidableshapes.Close(id);
		}
		return true;

	case 0x56f6d83c /* "edge" */:
		{
			CollisionPolygonDef def;
			ConfigureFixture(element, def.mFixture);
			ConfigureEdge(element, def.mShape);
			Database::Typed<CollisionShapeDef> &shapes = Database::collidableshapes.Open(id);
			if (const char *name = element->Attribute("name"))
				shapes.Put(Hash(name), def);
			else
				shapes.Put(shapes.GetCount() + 1, def);
			Database::collidableshapes.Close(id);
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureBody(const TiXmlElement *element, b2BodyDef &body, unsigned int id)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureBodyItem(child, body, id);
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

#ifdef B2_LINE_JOINT_H

bool CollidableTemplate::ConfigureLineJointItem(const TiXmlElement *element, b2LineJointDef &joint)
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

bool CollidableTemplate::ConfigureLineJoint(const TiXmlElement *element, b2LineJointDef &joint)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureLineJointItem(child, joint);
	}
	return true;
}

#endif

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
				CollidableTemplate::ConfigureBody(child, bodydef, id);
			}
			break;

		case 0xef2f9539 /* "revolutejoint" */:
			{
				b2RevoluteJointDef def;
				CollidableTemplate::ConfigureRevoluteJoint(child, def);
				Database::Typed<CollisionJointDef> &joints = Database::collidablejoints.Open(id);
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), def);
				else
					joints.Put(joints.GetCount() + 1, def);
				Database::collidablejoints.Close(id);
			}
			break;

		case 0x4954853d /* "prismaticjoint" */:
			{
				b2PrismaticJointDef def;
				CollidableTemplate::ConfigurePrismaticJoint(child, def);
				Database::Typed<CollisionJointDef> &joints = Database::collidablejoints.Open(id);
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), def);
				else
					joints.Put(joints.GetCount() + 1, def);
				Database::collidablejoints.Close(id);
			}
			break;

		case 0x6932d1ee /* "distancejoint" */:
			{
				b2DistanceJointDef def;
				CollidableTemplate::ConfigureDistanceJoint(child, def);
				Database::Typed<CollisionJointDef> &joints = Database::collidablejoints.Open(id);
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), def);
				else
					joints.Put(joints.GetCount() + 1, def);
				Database::collidablejoints.Close(id);
			}
			break;

		case 0xdd003dc4 /* "pulleyjoint" */:
			{
				b2PulleyJointDef def;
				CollidableTemplate::ConfigurePulleyJoint(child, def);
				Database::Typed<CollisionJointDef> &joints = Database::collidablejoints.Open(id);
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), def);
				else
					joints.Put(joints.GetCount() + 1, def);
				Database::collidablejoints.Close(id);
			}
			break;

#ifdef B2_LINE_JOINT_H
		case 0xa59c5ee9 /* "linejoint" */:
			{
				b2LineJointDef def;
				CollidableTemplate::ConfigureLineJoint(child, def);
				Database::Typed<CollisionJointDef> &joints = Database::collidablejoints.Open(id);
				if (const char *name = child->Attribute("name"))
					joints.Put(Hash(name), def);
				else
					joints.Put(joints.GetCount() + 1, def);
				Database::collidablejoints.Close(id);
			}
			break;
#endif
		}
	}

	return true;
}

bool CollidableTemplate::SetupLinkJoint(const LinkTemplate &linktemplate, unsigned int aId, unsigned int aSecondary)
{
	// add a revolute joint to the linked template (HACK)
	b2RevoluteJointDef def;
	
	// configure the joint definition
	def.body1 = reinterpret_cast<b2Body *>(aId);
	def.body2 = reinterpret_cast<b2Body *>(aSecondary);
	def.localAnchor1.Set(linktemplate.mOffset.p.x, linktemplate.mOffset.p.y);
	def.localAnchor2.Set(0, 0);
	def.referenceAngle = linktemplate.mOffset.Angle();
	if (linktemplate.mUpdateAngle)
	{
		def.lowerAngle = 0.0f;
		def.upperAngle = 0.0f;
		def.enableLimit = true;
	}

	Database::Typed<CollisionJointDef> &joints = Database::collidablejoints.Open(aSecondary);
	joints.Put(aId, def);
	Database::collidablejoints.Close(aSecondary);

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

	virtual void BeginContact(b2Contact* contact)
	{
		b2Fixture *fixture1 = contact->GetFixtureA();
		b2Fixture *fixture2 = contact->GetFixtureB();
		Database::Key id1 = reinterpret_cast<Database::Key>(fixture1->GetUserData());
		Database::Key id2 = reinterpret_cast<Database::Key>(fixture2->GetUserData());
		Database::collidablecontactadd.Get(id1)(id1, id2, 0.0f /*fixture1->GetBody()->m_sweep.t0*/, *contact);
		Database::collidablecontactadd.Get(id2)(id2, id1, 0.0f /*fixture2->GetBody()->m_sweep.t0*/, *contact);
	}
	virtual void EndContact(b2Contact* contact)
	{
		b2Fixture *fixture1 = contact->GetFixtureA();
		b2Fixture *fixture2 = contact->GetFixtureB();
		Database::Key id1 = reinterpret_cast<Database::Key>(fixture1->GetUserData());
		Database::Key id2 = reinterpret_cast<Database::Key>(fixture2->GetUserData());
		Database::collidablecontactremove.Get(id1)(id1, id2, 0.0f /*fixture1->GetBody()->m_sweep.t0*/, *contact);
		Database::collidablecontactremove.Get(id2)(id2, id1, 0.0f /*fixture2->GetBody()->m_sweep.t0*/, *contact);
	}
}
contactListener;


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
	void DrawTransform(const b2Transform& xf);
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

void DebugDraw::DrawTransform(const b2Transform& xf)
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

// create fixture visitor
class CollidableCreateFixture
	: public boost::static_visitor<b2Fixture *>
{
protected:
	b2Body *mBody;

public:
	CollidableCreateFixture(b2Body *aBody = NULL)
		: mBody(aBody)
	{
	}

	b2Fixture * operator()(const int & aDef) const
	{
		return NULL;
	}

	b2Fixture * operator()(const CollisionCircleDef & aDef) const
	{
		b2Fixture *fixture = mBody->CreateFixture(&aDef.mFixture);
		fixture->SetUserData(mBody->GetUserData());
		return fixture;
	}

	b2Fixture * operator()(const CollisionPolygonDef & aDef) const
	{
		b2Fixture *fixture = mBody->CreateFixture(&aDef.mFixture);
		fixture->SetUserData(mBody->GetUserData());
		return fixture;
	}

	/*
	b2Fixture * operator()(const b2EdgeChainDef & aDef) const
	{
		b2EdgeChainDef def(aDef);
		def.userData = mBody->GetUserData();
		return b2CreateEdgeChain(mBody, &def);
	}
	*/
};

// create joint visitor
class CollidableCreateJoint
	: public boost::static_visitor<b2Joint *>
{
protected:
	int id;

public:
	CollidableCreateJoint(int aId = 0)
		: id(id)
	{
	}

	b2Joint * operator()(const int & aDef) const
	{
		return NULL;
	}

	b2Joint * operator()(const b2RevoluteJointDef & aDef) const
	{
		b2RevoluteJointDef def(aDef);
		if (!Convert(def))
			return NULL;
		return Collidable::GetWorld()->CreateJoint(&def);
	}

	b2Joint * operator()(const b2PrismaticJointDef & aDef) const
	{
		b2PrismaticJointDef def(aDef);
		if (!Convert(def))
			return NULL;
		return Collidable::GetWorld()->CreateJoint(&def);
	}

	b2Joint * operator()(const b2DistanceJointDef & aDef) const
	{
		b2DistanceJointDef def(aDef);
		if (!Convert(def))
			return NULL;
		return Collidable::GetWorld()->CreateJoint(&def);
	}

	b2Joint * operator()(const b2PulleyJointDef & aDef) const
	{
		b2PulleyJointDef def(aDef);
		if (!Convert(def))
			return NULL;
		return Collidable::GetWorld()->CreateJoint(&def);
	}

	b2Joint * operator()(const b2LineJointDef & aDef) const
	{
		b2LineJointDef def(aDef);
		if (!Convert(def))
			return NULL;
		return Collidable::GetWorld()->CreateJoint(&def);
	}

protected:
	bool Convert(b2JointDef & aDef) const
	{
		unsigned int id1 = reinterpret_cast<unsigned int>(aDef.body1);
		aDef.userData = NULL;
		Collidable *coll1 = Database::collidable.Get(id1 ? id1 : id);
		if (!coll1)
			return false;
		aDef.body1 = coll1->GetBody();
		if (!aDef.body1)
			return false;
		unsigned int id2 = reinterpret_cast<unsigned int>(aDef.body2);
		Collidable *coll2 = Database::collidable.Get(id2 ? id2 : id);
		if (!coll2)
			return false;
		aDef.body2 = coll2->GetBody();
		if (!aDef.body2)
			return false;
		return true;
	}
};

void Collidable::AddToWorld(void)
{
	const CollidableTemplate &collidable = Database::collidabletemplate.Get(id);

	// copy the body definition
	b2BodyDef def(collidable.bodydef);

	// set userdata identifier and body position to entity (HACK)
	def.userData = reinterpret_cast<void *>(id);
	if (const Entity *entity = Database::entity.Get(id))
	{
		def.position = entity->GetPosition();
		def.angle = entity->GetAngle();
		def.linearVelocity = entity->GetVelocity();
		def.angularVelocity = entity->GetOmega();
	}

	// create the body
	body = world->CreateBody(&def);

	// add shapes
	for (Database::Typed<CollisionShapeDef>::Iterator itor(Database::collidableshapes.Find(id)); itor.IsValid(); ++itor)
		boost::apply_visitor(CollidableCreateFixture(body), itor.GetValue());

	// add joints
	for (Database::Typed<CollisionJointDef>::Iterator itor(Database::collidablejoints.Find(id)); itor.IsValid(); ++itor)
		boost::apply_visitor(CollidableCreateJoint(id), itor.GetValue());
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
	b2Vec2 gravity;
	gravity.Set(0.0f, 0.0f);
	bool doSleep = true;
	world = new b2World(gravity, doSleep);

	// set contact listener
	world->SetContactListener(&contactListener);

#ifdef COLLIDABLE_DEBUG_DRAW
	// set debug render
	world->SetDebugDraw(&debugDraw);
	debugDraw.SetFlags(~0U);
#endif

	if (aWall)
	{
		// create perimeter wall
		b2BodyDef def;
		b2Body *body = world->CreateBody(&def);

		b2Vec2 vertex[4] =
		{
			b2Vec2(aMaxX * (1.0f - FLT_EPSILON), aMaxY * (1.0f - FLT_EPSILON)),
			b2Vec2(aMaxX * (1.0f - FLT_EPSILON), aMinY * (1.0f - FLT_EPSILON)),
			b2Vec2(aMinX * (1.0f - FLT_EPSILON), aMinY * (1.0f - FLT_EPSILON)),
			b2Vec2(aMinX * (1.0f - FLT_EPSILON), aMaxY * (1.0f - FLT_EPSILON))
		};

		b2PolygonShape shape;
		for (int i = 0; i < 4; ++i)
		{
			shape.SetAsEdge(vertex[i], vertex[(i+1)&3]);
			body->CreateFixture(&shape);
		}
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

class CollidableRayCast : public b2RayCastCallback
{
public:
	b2Filter mFilter;
	unsigned int mSkipId;

	unsigned int mHitId;
	b2Fixture *mHitFixture;
	Vector2 mHitPoint;
	Vector2 mHitNormal;
	float mHitFraction;

public:
	CollidableRayCast(const b2Filter &aFilter, unsigned int aSkipId)
		: mFilter(aFilter), mSkipId(aSkipId), mHitId(0), mHitFixture(NULL), mHitFraction(1.0f)
	{
	}

	virtual float32 ReportFixture(	b2Fixture* fixture, const b2Vec2& point,
									const b2Vec2& normal, float32 fraction)
	{
		// skip unhittable fixtures
		if (fixture->IsSensor())
			return 1.0f;
		if (!Collidable::CheckFilter(mFilter, fixture->GetFilterData()))
			return 1.0f;

		// get the parent body
		b2Body* body = fixture->GetBody();

		// get the collidable identifier
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip self
		if (targetId == mSkipId)
			return 1.0f;

		// update hit entity identifier
		mHitId = reinterpret_cast<unsigned int>(body->GetUserData());

		// update hit fixture
		mHitFixture = fixture;

		// update hit location
		mHitPoint = point;
		mHitNormal = normal;
		mHitFraction = fraction;

		// get closest
		return fraction;
	}
};

unsigned int Collidable::TestSegment(const b2Segment &aSegment, const b2Filter &aFilter, unsigned int aId,
									 float &aLambda, b2Vec2 &aNormal, b2Fixture *&aShape)
{
	// raycast
	CollidableRayCast raycast(aFilter, aId);
	world->RayCast(&raycast, aSegment.p1, aSegment.p2);

	// return result
	aLambda = raycast.mHitFraction;
	aNormal = raycast.mHitNormal;
	return raycast.mHitId;
}

void Collidable::QueryAABB(b2QueryCallback* callback, const b2AABB& aabb)
{
	world->QueryAABB(callback, aabb);
}
