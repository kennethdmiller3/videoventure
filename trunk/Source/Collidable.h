#pragma once

#include "Database.h"

const int COLLISION_LAYERS = 32;

class LinkTemplate;

extern void ConfigureFilterData(b2FilterData &aFilter, const TiXmlElement *element);

class CollidableTemplate
{
public:
	// identifier
	unsigned int id;

	// body definition
	b2BodyDef bodydef;

#ifndef COLLIDABLE_SHAPE_DATABASE
	// shape definitions
	Database::Typed<b2ShapeDef *> shapes;
#endif

#ifndef COLLIDABLE_JOINT_DATABASE
	// joint definitions
	Database::Typed<b2JointDef *> joints;
#endif

public:
	CollidableTemplate(void);
	CollidableTemplate(const CollidableTemplate &aTemplate);
	~CollidableTemplate(void);

	// configure
	bool ProcessShapeItem(const TiXmlElement *element, b2ShapeDef &shape);
	bool ConfigureShape(const TiXmlElement *element, b2ShapeDef &shape);
	bool ConfigureCircle(const TiXmlElement *element, b2CircleDef &shape);
	bool ConfigureBox(const TiXmlElement *element, b2PolygonDef &shape);
	bool ProcessPolyItem(const TiXmlElement *element, b2PolygonDef &shape);
	bool ConfigurePoly(const TiXmlElement *element, b2PolygonDef &shape);
#ifdef B2_EDGE_SHAPE_H
	bool ConfigureEdge(const TiXmlElement *element, b2EdgeChainDef &shape);
#endif
	bool ProcessBodyItem(const TiXmlElement *element, b2BodyDef &body);
	bool ConfigureBody(const TiXmlElement *element, b2BodyDef &body);
	bool ProcessJointItem(const TiXmlElement *element, b2JointDef &joint);
	bool ProcessRevoluteJointItem(const TiXmlElement *element, b2RevoluteJointDef &joint);
	bool ConfigureRevoluteJoint(const TiXmlElement *element, b2RevoluteJointDef &joint);
	bool ProcessPrismaticJointItem(const TiXmlElement *element, b2PrismaticJointDef &joint);
	bool ConfigurePrismaticJoint(const TiXmlElement *element, b2PrismaticJointDef &joint);
	bool ProcessDistanceJointItem(const TiXmlElement *element, b2DistanceJointDef &joint);
	bool ConfigureDistanceJoint(const TiXmlElement *element, b2DistanceJointDef &joint);
	bool ProcessPulleyJointItem(const TiXmlElement *element, b2PulleyJointDef &joint);
	bool ConfigurePulleyJoint(const TiXmlElement *element, b2PulleyJointDef &joint);
	bool ProcessMouseJointItem(const TiXmlElement *element, b2MouseJointDef &joint);
	bool ConfigureMouseJoint(const TiXmlElement *element, b2MouseJointDef &joint);
	bool Configure(const TiXmlElement *element, unsigned int id);

	bool SetupLinkJoint(const LinkTemplate &linktemplate, unsigned int id, unsigned int secondary);
};

class Collidable
{
protected:
	static b2World *world;
	static b2AABB boundary;

	// identifier
	unsigned int id;

	// primary body
	b2Body *body;

public:
	typedef fastdelegate::FastDelegate<void (unsigned int, unsigned int, float, const b2ContactPoint &)> Listener;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Collidable(void);
	Collidable(const CollidableTemplate &aTemplate, unsigned int aId);
	virtual ~Collidable(void);

	unsigned int GetId()
	{
		return id;
	}

	void AddToWorld(void);
	void RemoveFromWorld(void);

	b2Body *GetBody(void) const
	{
		return body;
	}

	// collision world
	static void WorldInit(float aMinX, float aMinY, float aMaxX, float aMaxY);
	static void WorldDone(void);
	static b2World *GetWorld(void)
	{
		return world;
	}
	static const b2AABB &GetBoundary(void)
	{
		return boundary;
	}

	// default filter
	static const b2FilterData &GetDefaultFilter(void)
	{
		static b2FilterData filter = { 0x0001, 0xFFFF, 0 };
		return filter;
	}

	// check filtering
	static bool CheckFilter(const b2FilterData &aFilter1, const b2FilterData &aFilter2)
	{
		if (aFilter1.groupIndex == aFilter2.groupIndex && aFilter1.groupIndex != 0)
			return aFilter1.groupIndex > 0;
		return 
			(aFilter1.maskBits & aFilter2.categoryBits) != 0 && 
			(aFilter1.categoryBits & aFilter2.maskBits) != 0;
	}

	// test segment for intersection with world shapes
	static unsigned int TestSegment(const b2Segment &aSegment, const b2FilterData &aFilter, unsigned int aId,
									float &aLambda, b2Vec2 &aNormal, b2Shape *&aShape);

	// control
	static void CollideAll(float aStep);

protected:
	bool CreateJoint(const b2JointDef &joint) const;
};

namespace Database
{
	extern Typed<CollidableTemplate> collidabletemplate;
	extern Typed<Collidable *> collidable;
	extern Typed<Typed<Collidable::Listener> > collidablecontactadd;
	extern Typed<Typed<Collidable::Listener> > collidablecontactremove;
}
