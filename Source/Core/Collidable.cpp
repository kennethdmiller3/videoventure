#include "StdAfx.h"
#include "Collidable.h"
#include "Entity.h"
#include "Link.h"
#include "Command.h"
#include "Console.h"

// Chipmunk includes
#pragma message( "chipmunk" )
#include "chipmunk/chipmunk_private.h"
#include "chipmunk/chipmunk.h"

namespace Database
{
	Typed<CollidableFilter> collidablefilter(0x5224d988 /* "collidablefilter" */);
	Typed<CollidableTemplate> collidabletemplate(0xa7380c00 /* "collidabletemplate" */);
	Typed<Typed<CollidableCircleDef> > collidablecircles(0x67fa99ff /* "collidablecircles" */);
	Typed<Typed<CollidablePolygonDef> > collidablepolygons(0xab54c159 /* "collidablepolygons" */);
	Typed<Typed<CollidableEdgeDef> > collidableedges(0xae12a01c /* "collidableedges" */);
	Typed<Typed<CollidableChainDef> > collidablechains(0xd2bae418 /* "collidablechains" */);
	Typed<cpBody *> collidablebody(0x6ccc2b62 /* "collidablebody" */);
	Typed<Collidable::ContactSignal> collidablecontactadd(0x7cf2c45d /* "collidablecontactadd" */);
	Typed<Collidable::SeparateSignal> collidablecontactremove(0x95ed5aba /* "collidablecontactremove" */);

	namespace Loader
	{
		struct FilterDefault
		{
			FilterDefault()
			{
				CollidableFilter &filter = Database::collidablefilter.OpenDefault();
				filter = Collidable::GetDefaultFilter();
				Database::collidablefilter.CloseDefault();
			}
		}
		filterdefault;

		static void FilterConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			if (!Database::collidablefilter.Find(aId))
				Database::collidablefilter.Put(aId, Collidable::GetDefaultFilter());
			CollidableFilter &filter = Database::collidablefilter.Open(aId);
			ConfigureFilterData(filter, element);
			Database::collidablefilter.Close(aId);
		}
		Configure filterconfigure(0x5224d988 /* "collidablefilter" */, FilterConfigure);

		static void CollidableConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			CollidableTemplate &collidable = Database::collidabletemplate.Open(aId);
			collidable.Configure(element, aId);
			Database::collidabletemplate.Close(aId);
		}
		Configure collidableconfigure(0x74e9dbae /* "collidable" */, CollidableConfigure);
	}

	namespace Initializer
	{
		Activate collidableactivate(0xa7380c00 /* "collidabletemplate" */, Entry(Collidable::AddToWorld));
		Deactivate collidabledeactivate(0xa7380c00 /* "collidabletemplate" */, Entry(Collidable::RemoveFromWorld));
	}
}

static void ConfigureFilterCategory(CollidableFilter &aFilter, const tinyxml2::XMLElement *element, const char *name)
{
	int category = 0;
	if (element->QueryIntAttribute(name, &category) == tinyxml2::XML_SUCCESS)
	{
		if (category >= 0)
			aFilter.mCategories = 1U << category;
		else
			aFilter.mCategories = 0U;
	}
}

static void ConfigureFilterMask(CollidableFilter &aFilter, const tinyxml2::XMLElement *element)
{
	bool defvalue = true;
	if (element->QueryBoolAttribute("default", &defvalue) == tinyxml2::XML_SUCCESS)
		aFilter.mMask = defvalue ? ~0U : 0U;

	char buf[16];
	for (int i = 0; i < 32; i++)
	{
		sprintf(buf, "bit%d", i);
		bool bit = false;
		if (element->QueryBoolAttribute(buf, &bit) == tinyxml2::XML_SUCCESS)
		{
			if (bit)
				aFilter.mMask |= (1 << i);
			else
				aFilter.mMask &= ~(1 << i);
		}
	}
}

static void ConfigureFilterGroup(CollidableFilter &aFilter, const tinyxml2::XMLElement *element, const char *name)
{
	int group = 0;
	if (element->QueryIntAttribute(name, &group) == tinyxml2::XML_SUCCESS)
		aFilter.mGroup = -group;
}

#if 0
static void ConfigureFilterLayers(CollidableFilter &aFilter, const tinyxml2::XMLElement *element)
{
	bool defvalue = true;
	if (element->QueryBoolAttribute("default", &defvalue) == tinyxml2::XML_SUCCESS)
		aFilter.mLayers = defvalue ? ~0U : 0U;

	char buf[16];
	for (int i = 0; i < sizeof(aFilter.mLayers)*8; i++)
	{
		sprintf(buf, "bit%d", i);
		bool bit = false;
		if (element->QueryBoolAttribute(buf, &bit) == tinyxml2::XML_SUCCESS)
		{
			if (bit)
				aFilter.mLayers |= (1 << i);
			else
				aFilter.mLayers &= ~(1 << i);
		}
	}
}
#endif

void ConfigureFilterData(CollidableFilter &aFilter, const tinyxml2::XMLElement *element)
{
	if (const char *name = (Hash(element->Value()) == 0xc7e16877 /* "filter" */) ? element->Attribute("name") : element->Attribute("filter"))
		aFilter = Database::collidablefilter.Get(Hash(name));

	ConfigureFilterCategory(aFilter, element, "category");
	ConfigureFilterMask(aFilter, element);
	ConfigureFilterGroup(aFilter, element, "group");
#if 0
	ConfigureFilterLayers(aFilter, element);
#endif

	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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

#if 0
		case 0x8fb7915f /* "layers" */:
			ConfigureFilterLayers(aFilter, child);
			break;
#endif
		}
	}
}

CollidableTemplate::CollidableTemplate(void)
: mId(0)
, mBodyDef()
{
}

CollidableTemplate::CollidableTemplate(const CollidableTemplate &aTemplate)
: mId(aTemplate.mId)
, mBodyDef(aTemplate.mBodyDef)
{
}

CollidableTemplate::~CollidableTemplate(void)
{
}

bool CollidableTemplate::ConfigureShapeItem(const tinyxml2::XMLElement *element, CollidableShapeDef &shape)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xa51be2bb /* "friction" */:
		element->QueryFloatAttribute("value", &shape.mFriction);
		return true;

	case 0xf59a4f8f /* "restitution" */:
		element->QueryFloatAttribute("value", &shape.mElasticity);
		return true;

	case 0x72b9059b /* "density" */:
		element->QueryFloatAttribute("value", &shape.mDensity);
		return true;

	case 0xc7e16877 /* "filter" */:
		ConfigureFilterData(shape.mFilter, element);
		return true;

	case 0xcf2f4271 /* "category" */:
		ConfigureFilterCategory(shape.mFilter, element, "value");
		return true;

	case 0xe7774569 /* "mask" */:
		ConfigureFilterMask(shape.mFilter, element);
		return true;

	case 0x5fb91e8c /* "group" */:
		ConfigureFilterGroup(shape.mFilter, element, "value");
		return true;

#if 0
	case 0x8fb7915f /* "layers" */:
		ConfigureFilterLayers(shape.mFilter, element);
		return true;
#endif

	case 0x83b6367b /* "sensor" */:
		element->QueryBoolAttribute("value", &shape.mIsSensor);
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureShape(const tinyxml2::XMLElement *element, CollidableShapeDef &fixture)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureShapeItem(child, fixture);
	}
	return true;
}

bool CollidableTemplate::ConfigureCircle(const tinyxml2::XMLElement *element, CollidableCircleDef &shape)
{
	element->QueryFloatAttribute("radius", &shape.mRadius);
	return true;
}

bool CollidableTemplate::ConfigureBox(const tinyxml2::XMLElement *element, CollidablePolygonDef &shape)
{
	// half-width and half-height
	float w = 0, h = 0;
	element->QueryFloatAttribute("w", &w);
	element->QueryFloatAttribute("h", &h);

	// transform
	Transform2 transform(Transform2::Identity());

	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x934f4e0a /* "position" */:
			element->QueryFloatAttribute("x", &transform.p.x);
			element->QueryFloatAttribute("y", &transform.p.y);
			if (element->QueryFloatAttribute("angle", &transform.a) == tinyxml2::XML_SUCCESS)
				 transform.a *= float(M_PI)/180.0f;
			break;
		}
	}

	// generate vertices
	Matrix2 matrix(transform);
	shape.mVertices.reserve(4);
	shape.mVertices.push_back(matrix.Transform(Vector2(-w, -h)));
	shape.mVertices.push_back(matrix.Transform(Vector2( w, -h)));
	shape.mVertices.push_back(matrix.Transform(Vector2( w,  h)));
	shape.mVertices.push_back(matrix.Transform(Vector2(-w,  h)));

	return true;
}

bool CollidableTemplate::ConfigurePolyItem(const tinyxml2::XMLElement *element, CollidablePolygonDef &shape)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x945367a7 /* "vertex" */:
		{
			Vector2 v(0.0f, 0.0f);
			element->QueryFloatAttribute("x", &v.x);
			element->QueryFloatAttribute("y", &v.y);
			shape.mVertices.push_back(v);
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigurePoly(const tinyxml2::XMLElement *element, CollidablePolygonDef &shape)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigurePolyItem(child, shape);
	}

	return true;
}

bool CollidableTemplate::ConfigureEdge(const tinyxml2::XMLElement *element, CollidableEdgeDef &shape)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *name = child->Value();
		switch (Hash(name))
		{
		case 0x154c1122 /* "vertex1" */:
			child->QueryFloatAttribute("x", &shape.mA.x);
			child->QueryFloatAttribute("y", &shape.mA.y);
			break;

		case 0x144c0f8f /* "vertex2" */:
			child->QueryFloatAttribute("x", &shape.mB.x);
			child->QueryFloatAttribute("y", &shape.mB.y);
			break;
		}
	}

	return true;
}

bool CollidableTemplate::ConfigureChain(const tinyxml2::XMLElement *element, CollidableChainDef &shape)
{
	element->QueryBoolAttribute("loop", &shape.mLoop);

	for (const tinyxml2::XMLElement *child = element->FirstChildElement("vertex"); child != NULL; child = child->NextSiblingElement("vertex"))
	{
		Vector2 v(0.0f, 0.0f);
		child->QueryFloatAttribute("x", &v.x);
		child->QueryFloatAttribute("y", &v.y);
		shape.mVertices.push_back(v);
	}

	return true;
}


bool CollidableTemplate::ConfigureBodyItem(const tinyxml2::XMLElement *element, CollidableBodyDef &body, unsigned int id)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xdbaa7975 /* "body" */:
		// backwards compatibility
		ConfigureBody(element, body, id);
		return true;

	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &body.mTransform.p.x);
		element->QueryFloatAttribute("y", &body.mTransform.p.y);
		if (element->QueryFloatAttribute("angle", &body.mTransform.a) == tinyxml2::XML_SUCCESS)
			 body.mTransform.a *= float(M_PI)/180.0f;
		return true;

	case 0xbb61b895 /* "damping" */:
		element->QueryFloatAttribute("linear", &body.mLinearDamping);
		element->QueryFloatAttribute("angular", &body.mAngularDamping);
		return true;

	case 0x7a04061b /* "fixedrotation" */:
		element->QueryBoolAttribute("value", &body.mFixedRotation);
		return true;

	case 0x5127f14d /* "type" */:
		{
			switch(Hash(element->Attribute("value")))
			{
			case 0x923fa396 /* "auto" */:
				body.mType = CollidableBodyDef::kType_Auto;
				break;

			case 0xd290c23b /* "static" */:
				body.mType = CollidableBodyDef::kType_Static;
				break;

			case 0xc4be0946 /* "kinematic" */:
				body.mType = CollidableBodyDef::kType_Kinematic;
				break;

			case 0x4f5296ae /* "dynamic" */:
				body.mType = CollidableBodyDef::kType_Dynamic;
				break;
			}
		}
		return true;

	case 0x28217089 /* "circle" */:
		{
			Database::Typed<CollidableCircleDef> &shapes = Database::collidablecircles.Open(id);
			unsigned int subid;
			if (const char *name = element->Attribute("name"))
				subid = Hash(name);
			else
				subid = shapes.GetCount() + 1;
			CollidableCircleDef &def = shapes.Open(subid);
			ConfigureShape(element, def);
			ConfigureCircle(element, def);
			shapes.Close(subid);
			Database::collidablecircles.Close(id);
		}
		return true;

	case 0x70c67e32 /* "box" */:
		{
			Database::Typed<CollidablePolygonDef> &shapes = Database::collidablepolygons.Open(id);
			unsigned int subid;
			if (const char *name = element->Attribute("name"))
				subid = Hash(name);
			else
				subid = shapes.GetCount() + 1;
			CollidablePolygonDef &def = shapes.Open(subid);
			ConfigureShape(element, def);
			ConfigureBox(element, def);
			shapes.Close(subid);
			Database::collidablepolygons.Close(id);
		}
		return true;

	case 0x84d6a947 /* "poly" */:
		{
			Database::Typed<CollidablePolygonDef> &shapes = Database::collidablepolygons.Open(id);
			unsigned int subid;
			if (const char *name = element->Attribute("name"))
				subid = Hash(name);
			else
				subid = shapes.GetCount() + 1;
			CollidablePolygonDef &def = shapes.Open(subid);
			ConfigureShape(element, def);
			ConfigurePoly(element, def);
			shapes.Close(subid);
			Database::collidablepolygons.Close(id);
		}
		return true;

	case 0x56f6d83c /* "edge" */:
		{
			Database::Typed<CollidableEdgeDef> &shapes = Database::collidableedges.Open(id);
			unsigned int subid;
			if (const char *name = element->Attribute("name"))
				subid = Hash(name);
			else
				subid = shapes.GetCount() + 1;
			CollidableEdgeDef &def = shapes.Open(subid);
			ConfigureShape(element, def);
			ConfigureEdge(element, def);
			shapes.Close(subid);
			Database::collidableedges.Close(id);
		}
		return true;

	case 0x620c0b45 /* "edgechain" */:
		{
			Database::Typed<CollidableChainDef> &shapes = Database::collidablechains.Open(id);
			unsigned int subid;
			if (const char *name = element->Attribute("name"))
				subid = Hash(name);
			else
				subid = shapes.GetCount() + 1;
			CollidableChainDef &def = shapes.Open(subid);
			ConfigureShape(element, def);
			ConfigureChain(element, def);
			shapes.Close(subid);
			Database::collidablechains.Close(id);
		}
		return true;

	default:
		return false;
	}
}

bool CollidableTemplate::ConfigureBody(const tinyxml2::XMLElement *element, CollidableBodyDef &body, unsigned int id)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureBodyItem(child, body, id);
	}
	return true;
}

bool CollidableTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int id)
{
	// save identifier
	mId = id;

	// allow direct inclusion of body items
	ConfigureBody(element, mBodyDef, id);

	// now compute mass and moment
	mBodyDef.mMass = 0.0f;
	mBodyDef.mMoment = 0.0f;
	for (Database::Typed<CollidableCircleDef>::Iterator itor(Database::collidablecircles.Find(id)); itor.IsValid(); ++itor)
	{
		const CollidableCircleDef &def = itor.GetValue();
		float mass = def.mDensity * float(cpAreaForCircle(0.0f, def.mRadius));
		mBodyDef.mMass += mass;
		mBodyDef.mMoment += float(cpMomentForCircle(mass, 0.0f, def.mRadius, cpv(def.mOffset.x, def.mOffset.y)));
	}
	for (Database::Typed<CollidablePolygonDef>::Iterator itor(Database::collidablepolygons.Find(id)); itor.IsValid(); ++itor)
	{
		const CollidablePolygonDef &def = itor.GetValue();
		int count = int(def.mVertices.size());
		cpVect *verts = static_cast<cpVect *>(_alloca(sizeof(cpVect)*count));
		for (int i = 0; i < count; ++i)
			verts[i] = cpv(def.mVertices[i].x, def.mVertices[i].y);
		float mass = def.mDensity * float(cpAreaForPoly(count, verts, 0.0f));
		mBodyDef.mMass += mass;
		mBodyDef.mMoment += float(cpMomentForPoly(mass, count, verts, cpv(def.mOffset.x, def.mOffset.y), 0.0f));
	}

	return true;
}


namespace Collidable
{
	cpSpace* world;
	AlignedBox2 boundary;
	unsigned int poststepkey;
}

static void BodyUpdateVelocity(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
{
	Database::Key id = reinterpret_cast<Database::Key>(cpBodyGetUserData(body));
	const CollidableTemplate &collidable = Database::collidabletemplate.Get(id);
	const CollidableBodyDef &def = collidable.mBodyDef;

	body->f.x -= body->v.x * def.mLinearDamping * body->m;
	body->f.y -= body->v.y * def.mLinearDamping * body->m;
	body->t -= body->t * def.mAngularDamping * body->i;
	
	cpBodyUpdateVelocity(body, gravity, damping, dt);
}

static cpBool BeginContact(cpArbiter *arb, cpSpace *space, cpDataPointer data)
{
	CP_ARBITER_GET_SHAPES(arb, a, b);

	// arbiter begin contact
	cpBool retA = cpArbiterCallWildcardBeginA(arb, space);
	cpBool retB = cpArbiterCallWildcardBeginB(arb, space);
	if (!retA || !retB)
		return false;

	// signal contact add
	Database::Key id1 = reinterpret_cast<Database::Key>(cpShapeGetUserData(a));
	Database::Key id2 = reinterpret_cast<Database::Key>(cpShapeGetUserData(b));
	cpVect c = cpArbiterGetPointA(arb, 0);
	cpVect n = cpArbiterGetNormal(arb);
	const Vector2 contact(float(c.x), float(c.y));
	const Vector2 normal(float(n.x), float(n.y));
	Database::collidablecontactadd.Get(id1)(id1, id2, 0.0f, contact, normal);
	Database::collidablecontactadd.Get(id2)(id2, id1, 0.0f, contact, normal);
	return true;
}

static void EndContact(cpArbiter *arb, cpSpace *space, cpDataPointer data)
{
	CP_ARBITER_GET_SHAPES(arb, a, b);

	// signal contact end
	Database::Key id1 = reinterpret_cast<Database::Key>(cpShapeGetUserData(a));
	Database::Key id2 = reinterpret_cast<Database::Key>(cpShapeGetUserData(b));
	Database::collidablecontactremove.Get(id1)(id1, id2, 0.0f);
	Database::collidablecontactremove.Get(id2)(id2, id1, 0.0f);

	// arbiter end contact
	cpArbiterCallWildcardSeparateA(arb, space);
	cpArbiterCallWildcardSeparateB(arb, space);
}


#ifdef COLLIDABLE_DEBUG_DRAW

static bool DebugDrawCollidable = false;

static inline cpSpaceDebugColor RGBAColor(float r, float g, float b, float a){
	cpSpaceDebugColor color = { r, g, b, a };
	return color;
}

static inline cpSpaceDebugColor LAColor(float l, float a){
	cpSpaceDebugColor color = { l, l, l, a };
	return color;
}

static const float SHAPE_ALPHA = 0.75f;

static float DebugDrawPointLineScale = 1.0f;

static cpSpaceDebugColor
ColorFromHash(cpHashValue hash, float alpha)
{
	unsigned long val = (unsigned long)hash;
	
	// scramble the bits up using Robert Jenkins' 32 bit integer hash function
	val = (val+0x7ed55d16) + (val<<12);
	val = (val^0xc761c23c) ^ (val>>19);
	val = (val+0x165667b1) + (val<<5);
	val = (val+0xd3a2646c) ^ (val<<9);
	val = (val+0xfd7046c5) + (val<<3);
	val = (val^0xb55a4f09) ^ (val>>16);
	
	GLfloat r = unsigned char(val>>0);
	GLfloat g = unsigned char(val>>8);
	GLfloat b = unsigned char(val>>16);
	
	GLfloat max = std::max(std::max(r, g), b);
	GLfloat min = std::min(std::min(r, g), b);
	GLfloat intensity = 0.75f;
	
	// Saturate and scale the color
	if(min == max){
		return RGBAColor(intensity, 0.0f, 0.0f, alpha);
	} else {
		GLfloat coef = alpha*intensity/(max - min);
		return RGBAColor(
			(r - min)*coef,
			(g - min)*coef,
			(b - min)*coef,
			alpha
		);
	}
}

static inline void
glColor_from_color(cpSpaceDebugColor color){
	glColor4fv((GLfloat *)&color);
}

static cpSpaceDebugColor
ColorForShape(cpShape *shape, cpDataPointer data)
{
	if(cpShapeGetSensor(shape)){
		return LAColor(1, 0);
	} else {
		cpSpaceDebugColor color = ColorFromHash(cpShapeGetFilter(shape).mask /*CP_PRIVATE(hashid)*/, SHAPE_ALPHA);
		cpBody *body = cpShapeGetBody(shape);
		
		if(cpBodyIsSleeping(body)){
			color.r *= 0.2f;
			color.g *= 0.2f;
			color.b *= 0.2f;
		} else if(body->sleeping.idleTime > shape->space->sleepTimeThreshold) {
			color.r *= 0.66f;
			color.g *= 0.66f;
			color.b *= 0.66f;
		}
		return color;
	}
}

static const GLfloat circleVAR[] = {
	 0.0000f,  1.0000f,
	 0.2588f,  0.9659f,
	 0.5000f,  0.8660f,
	 0.7071f,  0.7071f,
	 0.8660f,  0.5000f,
	 0.9659f,  0.2588f,
	 1.0000f,  0.0000f,
	 0.9659f, -0.2588f,
	 0.8660f, -0.5000f,
	 0.7071f, -0.7071f,
	 0.5000f, -0.8660f,
	 0.2588f, -0.9659f,
	 0.0000f, -1.0000f,
	-0.2588f, -0.9659f,
	-0.5000f, -0.8660f,
	-0.7071f, -0.7071f,
	-0.8660f, -0.5000f,
	-0.9659f, -0.2588f,
	-1.0000f, -0.0000f,
	-0.9659f,  0.2588f,
	-0.8660f,  0.5000f,
	-0.7071f,  0.7071f,
	-0.5000f,  0.8660f,
	-0.2588f,  0.9659f,
	 0.0000f,  1.0000f,
	 0.0f, 0.0f, // For an extra line to see the rotation.
};
static const int circleVAR_count = sizeof(circleVAR)/sizeof(GLfloat)/2;

static void DebugDrawCircle(cpVect center, cpFloat angle, cpFloat radius, cpSpaceDebugColor lineColor, cpSpaceDebugColor fillColor, cpDataPointer data)
{
	glVertexPointer(2, GL_FLOAT, 0, circleVAR);

	glPushMatrix();

	glTranslatef(GLfloat(center.x), GLfloat(center.y), 0.0f);
	glRotatef(GLfloat(angle*180.0f/M_PI), 0.0f, 0.0f, 1.0f);
	glScalef(GLfloat(radius), GLfloat(radius), 1.0f);

	if(fillColor.a > 0)
	{
		glColor_from_color(fillColor);
		glDrawArrays(GL_TRIANGLE_FAN, 0, circleVAR_count - 1);
	}

	if(lineColor.a > 0)
	{
		glColor_from_color(lineColor);
		glDrawArrays(GL_LINE_STRIP, 0, circleVAR_count);
	}

	glPopMatrix();
}

static const GLfloat pillVAR[] = {
	 0.0000f,  1.0000f, 1.0f,
	 0.2588f,  0.9659f, 1.0f,
	 0.5000f,  0.8660f, 1.0f,
	 0.7071f,  0.7071f, 1.0f,
	 0.8660f,  0.5000f, 1.0f,
	 0.9659f,  0.2588f, 1.0f,
	 1.0000f,  0.0000f, 1.0f,
	 0.9659f, -0.2588f, 1.0f,
	 0.8660f, -0.5000f, 1.0f,
	 0.7071f, -0.7071f, 1.0f,
	 0.5000f, -0.8660f, 1.0f,
	 0.2588f, -0.9659f, 1.0f,
	 0.0000f, -1.0000f, 1.0f,

	 0.0000f, -1.0000f, 0.0f,
	-0.2588f, -0.9659f, 0.0f,
	-0.5000f, -0.8660f, 0.0f,
	-0.7071f, -0.7071f, 0.0f,
	-0.8660f, -0.5000f, 0.0f,
	-0.9659f, -0.2588f, 0.0f,
	-1.0000f, -0.0000f, 0.0f,
	-0.9659f,  0.2588f, 0.0f,
	-0.8660f,  0.5000f, 0.0f,
	-0.7071f,  0.7071f, 0.0f,
	-0.5000f,  0.8660f, 0.0f,
	-0.2588f,  0.9659f, 0.0f,
	 0.0000f,  1.0000f, 0.0f,
};
static const int pillVAR_count = sizeof(pillVAR)/sizeof(GLfloat)/3;

static void DebugDrawSegment(cpVect a, cpVect b, cpSpaceDebugColor color, cpDataPointer data)
{
	GLfloat verts[] =
	{
		GLfloat(a.x), GLfloat(a.y),
		GLfloat(b.x), GLfloat(b.y),
	};
	
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glColor_from_color(color);
	glDrawArrays(GL_LINES, 0, 2);
}

static void DebugDrawFatSegment(cpVect a, cpVect b, cpFloat radius, cpSpaceDebugColor lineColor, cpSpaceDebugColor fillColor, cpDataPointer data)
{
	if(radius)
	{
		glVertexPointer(3, GL_FLOAT, 0, pillVAR);

		glPushMatrix();

		cpVect d = cpvsub(b, a);
		cpVect r = cpvmult(d, radius/cpvlength(d));

		const GLfloat matrix[] =
		{
			GLfloat(r.x), GLfloat(r.y), 0.0f, 0.0f,
			-GLfloat(r.y), GLfloat(r.x), 0.0f, 0.0f,
			GLfloat(d.x), GLfloat(d.y), 0.0f, 0.0f,
			GLfloat(a.x), GLfloat(a.y), 0.0f, 1.0f,
		};
		glMultMatrixf(matrix);
			
		if(fillColor.a > 0)
		{
			glColor_from_color(fillColor);
			glDrawArrays(GL_TRIANGLE_FAN, 0, pillVAR_count);
		}
			
		if(lineColor.a > 0)
		{
			glColor_from_color(lineColor);
			glDrawArrays(GL_LINE_LOOP, 0, pillVAR_count);
		}

		glPopMatrix();
	}
	else
	{
		DebugDrawSegment(a, b, lineColor, data);
	}
}

static void DebugDrawPolygon(int count, const cpVect *verts, cpFloat radius, cpSpaceDebugColor lineColor, cpSpaceDebugColor fillColor, cpDataPointer data)
{
#if CP_USE_DOUBLES
	glVertexPointer(2, GL_DOUBLE, 0, verts);
#else
	glVertexPointer(2, GL_FLOAT, 0, verts);
#endif
	
	if(fillColor.a > 0)
	{
		glColor_from_color(fillColor);
		glDrawArrays(GL_TRIANGLE_FAN, 0, count);
	}
	
	if(lineColor.a > 0)
	{
		glColor_from_color(lineColor);
		glDrawArrays(GL_LINE_LOOP, 0, count);
	}
}

static void DebugDrawDot(cpFloat size, cpVect pos, cpSpaceDebugColor color, cpDataPointer data)
{
#if CP_USE_DOUBLES
	glVertexPointer(2, GL_DOUBLE, 0, &pos);
#else
	glVertexPointer(2, GL_FLOAT, 0, verts);
#endif
	
	glPointSize(GLfloat(size)*DebugDrawPointLineScale);
	glColor_from_color(color);
	glDrawArrays(GL_POINTS, 0, 1);
}

static void DebugDrawShapes(cpSpace *space)
{
	cpSpaceDebugDrawOptions drawOptions = {
		DebugDrawCircle,
		DebugDrawSegment,
		DebugDrawFatSegment,
		DebugDrawPolygon,
		DebugDrawDot,

		(cpSpaceDebugDrawFlags)(CP_SPACE_DEBUG_DRAW_SHAPES | CP_SPACE_DEBUG_DRAW_CONSTRAINTS | CP_SPACE_DEBUG_DRAW_COLLISION_POINTS),

		{ 200.0f / 255.0f, 210.0f / 255.0f, 230.0f / 255.0f, 1.0f },
		ColorForShape,
		{ 0.0f, 0.75f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		NULL,
	};

	cpSpaceDebugDraw(space, &drawOptions);
}

// console
extern Console *console;

int CommandDrawCollidable(const char * const aParam[], int aCount)
{
#if 1
	if (aCount >= 1)
		DebugDrawCollidable = atoi(aParam[0]) != 0;
	else
		console->Print("drawcollidable: %s\n", DebugDrawCollidable ? "on" : "off");
	return std::min(aCount, 1);
#else
	struct Option
	{
		unsigned int hash;
		const char * name;
		unsigned int flag;
	};
	const Option options[] =
	{
		{ 0x9dc3d926 /* "shape" */, "shape", b2Draw::e_shapeBit },
		{ 0xaeae0877 /* "joint" */, "joint", b2Draw::e_jointBit },
		{ 0x63e91357 /* "aabb" */, "aabb", b2Draw::e_aabbBit },
		{ 0x7c445ab1 /* "pair" */, "pair", b2Draw::e_pairBit },
		{ 0x058c4484 /* "center" */, "center", b2Draw::e_centerOfMassBit },
	};

	unsigned int hash = (aCount >= 1) ? Hash(aParam[0]) : 0x13254bc4 /* "all" */;
	for (int i = 0; i < SDL_arraysize(options); ++i)
	{
		const Option &option = options[i];
		if (hash == option.hash || hash == 0x13254bc4 /* "all" */)
		{
			if (aCount >= 2)
			{
				if (atoi(aParam[1]) != 0)
					debugDraw.AppendFlags(option.flag);
				else
					debugDraw.ClearFlags(option.flag);
			}
			else
			{
				console->Print("%s: %s\n", option.name, (debugDraw.GetFlags() & option.flag) == option.flag ? "on" : "off");
			}
		}
	}

	return std::min(aCount, 2);
#endif
}
Command commanddrawcollidable(0x38c5ac70 /* "drawcollidable" */, CommandDrawCollidable);

#endif

cpSpace *Collidable::GetWorld(void)
{
	return world;
}
const AlignedBox2 &Collidable::GetBoundary(void)
{
	return boundary;
}

static void PostStepAddToWorld(cpSpace *space, void *key, void *id)
{
	Collidable::AddToWorld(reinterpret_cast<Database::Key>(id));
}

void Collidable::AddToWorld(Database::Key aId)
{
	if (cpSpaceIsLocked(world))
	{
		cpSpaceAddPostStepCallback(world, PostStepAddToWorld, reinterpret_cast<void *>(poststepkey++), reinterpret_cast<void *>(aId));
		return;
	}

	const CollidableTemplate &collidable = Database::collidabletemplate.Get(aId);

	// copy the body definition
	CollidableBodyDef def(collidable.mBodyDef);

	// set body position to entity (HACK)
	if (const Entity *entity = Database::entity.Get(aId))
	{
		def.mTransform = entity->GetTransform();
		def.mVelocity.p = entity->GetVelocity();
		def.mVelocity.a = entity->GetOmega();
	}

	// if using automatic type...
	if (def.mType == CollidableBodyDef::kType_Auto)
	{
		// dynamic if the body has mass; static otherwise
		if (def.mMass > 0.0f)
			def.mType = CollidableBodyDef::kType_Dynamic;
		else
			def.mType = CollidableBodyDef::kType_Static;
	}

	// get the body
	cpBody *body = (def.mType == CollidableBodyDef::kType_Static)
		? cpBodyNewStatic()
		: cpBodyNew(def.mMass, def.mMoment);

	cpBodySetPosition(body, cpv(def.mTransform.p.x, def.mTransform.p.y));
	cpBodySetAngle(body, def.mTransform.a);
	cpBodySetVelocity(body, cpv(def.mVelocity.p.x, def.mVelocity.p.y));
	cpBodySetAngularVelocity(body, def.mVelocity.a);

	cpBodySetUserData(body, reinterpret_cast<void *>(aId));
	//if (def.mFixedRotation)
	//	cpBodySetAngularVelocityLimit(body, 0.0f);

	if (def.mType == CollidableBodyDef::kType_Dynamic)
	{
		if (def.mLinearDamping || def.mAngularDamping)
			body->velocity_func = BodyUpdateVelocity;
		cpSpaceAddBody(world, body);
	}

	Database::collidablebody.Put(aId, body);

	// add shapes
	for (Database::Typed<CollidableCircleDef>::Iterator itor(Database::collidablecircles.Find(aId)); itor.IsValid(); ++itor)
	{
		const CollidableCircleDef &def = itor.GetValue();
		cpShape *shape = cpCircleShapeNew(body,
			def.mRadius, cpv(def.mOffset.x, def.mOffset.y));
		cpShapeSetSensor(shape, def.mIsSensor);
		cpShapeSetFriction(shape, def.mFriction);
		cpShapeSetElasticity(shape, def.mElasticity);
		cpShapeSetSurfaceVelocity(shape, cpv(def.mSurfaceVelocity.x, def.mSurfaceVelocity.y));
		cpShapeSetUserData(shape, reinterpret_cast<void *>(aId));
		cpShapeSetFilter(shape, cpShapeFilterNew(def.mFilter.mGroup, def.mFilter.mCategories, def.mFilter.mMask));
		cpSpaceAddShape(world, shape);
	}
	for (Database::Typed<CollidablePolygonDef>::Iterator itor(Database::collidablepolygons.Find(aId)); itor.IsValid(); ++itor)
	{
		const CollidablePolygonDef &def = itor.GetValue();
		int count = int(def.mVertices.size());
		cpVect *verts = static_cast<cpVect *>(_alloca(sizeof(cpVect)*count));
		for (int i = 0; i < count; ++i)
			verts[i] = cpv(def.mVertices[count-1-i].x, def.mVertices[count-1-i].y);
		cpShape *shape = cpPolyShapeNew(body,
			count, verts, cpTransformNew(1.0f, 0.0f, 0.0f, 1.0f, def.mOffset.x, def.mOffset.y), 0.0f);
		cpShapeSetSensor(shape, def.mIsSensor);
		cpShapeSetFriction(shape, def.mFriction);
		cpShapeSetElasticity(shape, def.mElasticity);
		cpShapeSetSurfaceVelocity(shape, cpv(def.mSurfaceVelocity.x, def.mSurfaceVelocity.y));
		cpShapeSetUserData(shape,reinterpret_cast<void *>(aId));
		cpShapeSetFilter(shape, cpShapeFilterNew(def.mFilter.mGroup, def.mFilter.mCategories, def.mFilter.mMask));
		cpSpaceAddShape(world, shape);
	}
	for (Database::Typed<CollidableEdgeDef>::Iterator itor(Database::collidableedges.Find(aId)); itor.IsValid(); ++itor)
	{
		const CollidableEdgeDef &def = itor.GetValue();
		cpShape *shape = cpSegmentShapeNew(body,
			cpv(def.mA.x, def.mA.y), cpv(def.mB.x, def.mB.y), def.mRadius);
		cpShapeSetSensor(shape, def.mIsSensor);
		cpShapeSetFriction(shape, def.mFriction);
		cpShapeSetElasticity(shape, def.mElasticity);
		cpShapeSetSurfaceVelocity(shape, cpv(def.mSurfaceVelocity.x, def.mSurfaceVelocity.y));
		cpShapeSetUserData(shape, reinterpret_cast<void *>(aId));
		cpShapeSetFilter(shape, cpShapeFilterNew(def.mFilter.mGroup, def.mFilter.mCategories, def.mFilter.mMask));
		cpSpaceAddShape(world, shape);
	}
	for (Database::Typed<CollidableChainDef>::Iterator itor(Database::collidablechains.Find(aId)); itor.IsValid(); ++itor)
	{
		const CollidableChainDef &def = itor.GetValue();
		int count = int(def.mVertices.size());
		cpVect a = def.mLoop
			? cpv(def.mVertices[count-1].x, def.mVertices[count-1].y)
			: cpv(def.mVertices[0].x, def.mVertices[0].y);
		for (int i = def.mLoop ? 0 : 1; i < count; ++i)
		{
			cpVect b = cpv(def.mVertices[i].x, def.mVertices[i].y);
			cpShape *shape = cpSegmentShapeNew(body, a, b, def.mRadius);
			cpShapeSetSensor(shape, def.mIsSensor);
			cpShapeSetFriction(shape, def.mFriction);
			cpShapeSetElasticity(shape, def.mElasticity);
			cpShapeSetSurfaceVelocity(shape, cpv(def.mSurfaceVelocity.x, def.mSurfaceVelocity.y));
			cpShapeSetUserData(shape, reinterpret_cast<void *>(aId));
			cpSpaceAddShape(world, shape);
			cpShapeSetFilter(shape, cpShapeFilterNew(def.mFilter.mGroup, def.mFilter.mCategories, def.mFilter.mMask));
			a = b;
		}
	}
}

static void RemoveShapeFromWorld(cpBody *body, cpShape *shape, void *data)
{
	if (cpSpace *space = cpShapeGetSpace(shape))
		cpSpaceRemoveShape(space, shape);
	cpShapeFree(shape);
}

static void RemoveConstraintFromWorld(cpBody *body, cpConstraint *constraint, void *data)
{
	if (cpSpace *space = cpConstraintGetSpace(constraint))
		cpSpaceRemoveConstraint(space, constraint);
	cpConstraintFree(constraint);
}

static void RemoveBodyFromWorld(cpBody *body)
{
	cpBodyEachShape(body, RemoveShapeFromWorld, NULL);
	cpBodyEachConstraint(body, RemoveConstraintFromWorld, NULL);
	if (cpSpace *space = cpBodyGetSpace(body))
		cpSpaceRemoveBody(space, body);
	cpBodyFree(body);
}

static void PostStepRemoveFromWorld(cpSpace *space, void *key, void *id)
{
	Collidable::RemoveFromWorld(reinterpret_cast<unsigned int>(id));
}

void Collidable::RemoveFromWorld(Database::Key aId)
{
	if (cpSpaceIsLocked(world))
	{
		cpSpaceAddPostStepCallback(world, PostStepRemoveFromWorld, reinterpret_cast<void *>(poststepkey++), reinterpret_cast<void *>(aId));
		return;
	}
	if (cpBody *body = Database::collidablebody.Get(aId))
	{
		RemoveBodyFromWorld(body);
	}
	Database::collidablebody.Delete(aId);
	Database::collidablecontactadd.Delete(aId);
	Database::collidablecontactremove.Delete(aId);
}



// create collision world
void Collidable::WorldInit(float aMinX, float aMinY, float aMaxX, float aMaxY, bool aWall)
{
	// save boundary extents
	boundary.min.x = aMinX;
	boundary.min.y = aMinY;
	boundary.max.x = aMaxX;
	boundary.max.y = aMaxY;

	// create physics world
	world = cpSpaceNew();
	cpSpaceSetSleepTimeThreshold(world, 1.0f);
	cpSpaceSetCollisionSlop(world, 0.0f);	//0.01);
	cpSpaceSetCollisionBias(world, powf(0.5f, 60.0f));

	// set default collision handler
	cpCollisionHandler *handler = cpSpaceAddDefaultCollisionHandler(world);
	handler->beginFunc = BeginContact;
	handler->separateFunc = EndContact;

#ifdef COLLIDABLE_DEBUG_DRAW
	// set debug render
	//world->SetDebugDraw(&debugDraw);
#endif

	if (aWall)
	{
		// create perimeter wall
		const Vector2 vertex[4] =
		{
			Vector2(aMinX, aMinY),
			Vector2(aMaxX, aMinY),
			Vector2(aMaxX, aMaxY),
			Vector2(aMinX, aMaxY)
		};
		for (int i = 0; i < 4; ++i)
		{
			const Vector2 &a = vertex[i];
			const Vector2 &b = vertex[(i+1)&3];
			cpShape *wall = cpSegmentShapeNew(world->staticBody, cpv(a.x, a.y), cpv(b.x, b.y), 0.0f);
			cpShapeSetFilter(wall, cpShapeFilterNew(0, ~0U, ~0U));
			cpShapeSetFriction(wall, 0.2);
			cpSpaceAddShape(world, wall);
		}
	}
}

void Collidable::WorldDone(void)
{
	cpSpaceFree(world);
	world = NULL;
}

void Collidable::CollideAll(float aStep)
{
	// exit if no world
	if (!world)
		return;

	// step the physics world
	cpSpaceStep(world, aStep);

	// for each body...
	for (Database::Typed<cpBody *>::Iterator i(&Database::collidablebody); i.IsValid(); ++i)
	{
		// get the body
		cpBody *body = i.GetValue();

		// if the body is not sleeping or static...
		if (!cpBodyIsSleeping(body) && cpBodyGetType(body) != CP_BODY_TYPE_STATIC)
		{
			// update the entity position (hack)
			Database::Key id = i.GetKey();
			Entity *entity = Database::entity.Get(id);
			if (entity)
			{
				entity->Step();
				cpFloat ang(cpBodyGetAngle(body));
				cpVect pos(cpBodyGetPosition(body));
				cpFloat omg(cpBodyGetAngularVelocity(body));
				cpVect vel(cpBodyGetVelocity(body));
				entity->SetTransform(float(ang), Vector2(float(pos.x), float(pos.y)));
				entity->SetVelocity(Vector2(float(vel.x), float(vel.y)));
				entity->SetOmega(float(omg));
			}
		}
	}

#ifdef COLLIDABLE_DEBUG_DRAW
	//world->DrawDebugData();
	if (DebugDrawCollidable)
		DebugDrawShapes(world);
#endif
}

class CollidableRayCast
{
public:
	CollidableFilter mFilter;
	unsigned int mSkipId;

	unsigned int mHitId;
	cpShape *mHitShape;
	Vector2 mHitNormal;
	float mHitFraction;

public:
	CollidableRayCast(const CollidableFilter &aFilter, unsigned int aSkipId)
		: mFilter(aFilter), mSkipId(aSkipId), mHitId(0), mHitShape(NULL), mHitFraction(1.0f)
	{
	}

	static void Query(cpShape *shape, cpVect point, cpVect normal, cpFloat fraction, void *data)
	{
		static_cast<CollidableRayCast *>(data)->Report(shape, float(fraction), Vector2(float(normal.x), float(normal.y)));
	}

	virtual void Report(CollidableShape* shape, float fraction, const Vector2& normal)
	{
		// ignore hits beyond the best hit
		if (fraction > mHitFraction)
			return;

		// skip unhittable fixtures
		if (Collidable::IsSensor(shape))
			return;
		if (!Collidable::CheckFilter(mFilter, Collidable::GetFilter(shape)))
			return;

		// get the collidable identifier
		unsigned int targetId = Collidable::GetId(shape);

		// skip self
		if (targetId == mSkipId)
			return;

		// update hit entity identifier
		mHitId = targetId;

		// update hit fixture
		mHitShape = shape;

		// update hit location
		mHitNormal = normal;
		mHitFraction = fraction;
	}
};

unsigned int Collidable::TestSegment(const Vector2 &aStart, const Vector2 &aEnd, const CollidableFilter &aFilter, unsigned int aId,
									 float &aLambda, Vector2 &aNormal, cpShape *&aShape)
{
	// pad the segment
	const Vector2 pad(0.125f * InvSqrt(aStart.DistSq(aEnd) + FLT_MIN) * (aEnd - aStart));

	// find the closest hit
	CollidableRayCast raycast(aFilter, aId);
	cpSpaceSegmentQuery(world, cpv(aStart.x - pad.x, aStart.y - pad.y), cpv(aEnd.x + pad.x, aEnd.y + pad.y), 0.0f,
		cpShapeFilterNew(aFilter.mGroup, aFilter.mCategories, aFilter.mMask), CollidableRayCast::Query, &raycast);

	// return result
	aLambda = raycast.mHitFraction;
	aNormal = raycast.mHitNormal;
	return raycast.mHitId;
}

static void QueryBoxCallback(cpShape *shape, void *data)
{
	(*static_cast<Collidable::QueryBoxDelegate *>(data))(shape);
}

void Collidable::QueryBox(const AlignedBox2 &aBox, const CollidableFilter &aFilter, QueryBoxDelegate aDelegate)
{
	cpSpaceBBQuery(world, cpBBNew(aBox.min.x, aBox.min.y, aBox.max.x, aBox.max.y), cpShapeFilterNew(aFilter.mGroup, aFilter.mCategories, aFilter.mMask), QueryBoxCallback, &aDelegate); 
}

static void QueryRadiusCallback(cpShape *shape, cpVect point, cpFloat distance, cpVect gradient, cpDataPointer data)
{
	(*static_cast<Collidable::QueryRadiusDelegate *>(data))(shape, float(distance), Vector2(float(point.x), float(point.y)));
}

// 
void Collidable::QueryRadius(const Vector2 &aCenter, float aRadius, const CollidableFilter &aFilter, QueryRadiusDelegate aDelegate)
{
	cpSpacePointQuery(world, cpv(aCenter.x, aCenter.y), aRadius, cpShapeFilterNew(aFilter.mGroup, aFilter.mCategories, aFilter.mMask), QueryRadiusCallback, &aDelegate);
}

// is a shape a sensor?
bool Collidable::IsSensor(CollidableShape *aShape)
{
	return cpShapeGetSensor(aShape) != 0;
}

// get the collision filter for a shape
CollidableFilter Collidable::GetFilter(CollidableShape *aShape)
{
	const cpShapeFilter filter(cpShapeGetFilter(aShape));
	return CollidableFilter(filter.group, filter.categories, filter.mask);
}

// get the owner id of a shape
unsigned int Collidable::GetId(CollidableShape *aShape)
{
	return reinterpret_cast<unsigned int>(cpShapeGetUserData(aShape));
}

// get the center of a shape
Vector2 Collidable::GetCenter(CollidableShape *aShape)
{
	cpBB box = cpShapeGetBB(aShape);
	return Vector2(float(box.l + box.r) * 0.5f, float(box.b + box.t) * 0.5f);
}

// set the position of a body
void Collidable::SetPosition(CollidableBody *aBody, const Vector2 &aPosition)
{
	cpBodySetPosition(aBody, cpv(aPosition.x, aPosition.y));
}

// set the angle of a body
void Collidable::SetAngle(CollidableBody *aBody, const float aAngle)
{
	cpBodySetAngle(aBody, aAngle);
}

// set the linear velocity of a body
void Collidable::SetVelocity(CollidableBody *aBody, const Vector2 &aVelocity)
{
	cpBodySetVelocity(aBody, cpv(aVelocity.x, aVelocity.y));
}

// add linear velocity to a body
void Collidable::AddVelocity(CollidableBody *aBody, const Vector2 &aDelta)
{
	cpBodySetVelocity(aBody, cpvadd(cpBodyGetVelocity(aBody), cpv(aDelta.x, aDelta.y)));
}

// set the angular velocity of a body
void Collidable::SetOmega(CollidableBody *aBody, const float aOmega)
{
	cpBodySetAngularVelocity(aBody, aOmega);
}

// add angular velocity to a body
void Collidable::AddOmega(CollidableBody *aBody, const float aDelta)
{
	cpBodySetAngularVelocity(aBody, cpBodyGetAngularVelocity(aBody) + aDelta);
}

// apply an impulse to a body
void Collidable::ApplyImpulse(CollidableBody *aBody, const Vector2 &aImpulse)
{
	cpBodyApplyImpulseAtLocalPoint(aBody, cpv(aImpulse.x, aImpulse.y), cpvzero);
}

