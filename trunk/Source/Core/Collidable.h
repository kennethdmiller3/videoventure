#pragma once

#include "Database.h"
#include "Signal.h"

const int COLLISION_LAYERS = 32;

// TO DO: configure this from the level file
const float sLengthUnitsPerMeter = 16.0f;
const float sMetersPerLengthUnit = 1.0f/sLengthUnitsPerMeter;

class LinkTemplate;

extern GAME_API void ConfigureFilterData(b2Filter &aFilter, const TiXmlElement *element);

class GAME_API CollidableTemplate
{
public:
	// identifier
	unsigned int id;

	// body definition
	b2BodyDef bodydef;

public:
	CollidableTemplate(void);
	CollidableTemplate(const CollidableTemplate &aTemplate);
	~CollidableTemplate(void);

	// configure
	bool ConfigureFixtureItem(const TiXmlElement *element, b2FixtureDef &fixture);
	bool ConfigureFixture(const TiXmlElement *element, b2FixtureDef &fixture);
	bool ConfigureCircle(const TiXmlElement *element, b2CircleShape &shape);
	bool ConfigureBox(const TiXmlElement *element, b2PolygonShape &shape);
	bool ConfigurePolyItem(const TiXmlElement *element, b2PolygonShape &shape);
	bool ConfigurePoly(const TiXmlElement *element, b2PolygonShape &shape);
	bool ConfigureEdge(const TiXmlElement *element, b2PolygonShape &shape);
	bool ConfigureBodyItem(const TiXmlElement *element, b2BodyDef &body, unsigned int id);
	bool ConfigureBody(const TiXmlElement *element, b2BodyDef &body, unsigned int id);
	bool ConfigureJointItem(const TiXmlElement *element, b2JointDef &joint);
	bool ConfigureRevoluteJointItem(const TiXmlElement *element, b2RevoluteJointDef &joint);
	bool ConfigureRevoluteJoint(const TiXmlElement *element, b2RevoluteJointDef &joint);
	bool ConfigurePrismaticJointItem(const TiXmlElement *element, b2PrismaticJointDef &joint);
	bool ConfigurePrismaticJoint(const TiXmlElement *element, b2PrismaticJointDef &joint);
	bool ConfigureDistanceJointItem(const TiXmlElement *element, b2DistanceJointDef &joint);
	bool ConfigureDistanceJoint(const TiXmlElement *element, b2DistanceJointDef &joint);
	bool ConfigurePulleyJointItem(const TiXmlElement *element, b2PulleyJointDef &joint);
	bool ConfigurePulleyJoint(const TiXmlElement *element, b2PulleyJointDef &joint);
#ifdef B2_LINE_JOINT_H
	bool ConfigureLineJointItem(const TiXmlElement *element, b2LineJointDef &joint);
	bool ConfigureLineJoint(const TiXmlElement *element, b2LineJointDef &joint);
#endif
	bool Configure(const TiXmlElement *element, unsigned int id);

	bool SetupLinkJoint(const LinkTemplate &linktemplate, unsigned int id, unsigned int secondary);
};

class GAME_API Collidable
{
protected:
	static b2World *world;
	static b2AABB boundary;

	// identifier
	unsigned int id;

	// primary body
	b2Body *body;

public:
	typedef Signal<void (unsigned int id1, unsigned int id2, float t, const b2Contact &contact)> ContactSignal;

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
	static void WorldInit(float aMinX, float aMinY, float aMaxX, float aMaxY, bool aWall);
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
	static const b2Filter &GetDefaultFilter(void)
	{
		static b2Filter filter = { 0x0001, 0xFFFF, 0 };
		return filter;
	}

	// check filtering
	static bool CheckFilter(const b2Filter &aFilter1, const b2Filter &aFilter2)
	{
		if (aFilter1.groupIndex == aFilter2.groupIndex && aFilter1.groupIndex != 0)
			return aFilter1.groupIndex > 0;
		return 
			(aFilter1.maskBits & aFilter2.categoryBits) != 0 && 
			(aFilter1.categoryBits & aFilter2.maskBits) != 0;
	}

	// test segment for intersection with world shapes
	static unsigned int TestSegment(const b2Segment &aSegment, const b2Filter &aFilter, unsigned int aId,
									float &aLambda, b2Vec2 &aNormal, b2Fixture *&aShape);

	// wrapper for world query aabb
	static void QueryAABB(b2QueryCallback* callback, const b2AABB& aabb);

	// control
	static void CollideAll(float aStep);
};

namespace Database
{
	extern GAME_API Typed<CollidableTemplate> collidabletemplate;
	extern GAME_API Typed<Collidable *> collidable;
	extern GAME_API Typed<Collidable::ContactSignal> collidablecontactadd;
	extern GAME_API Typed<Collidable::ContactSignal> collidablecontactremove;
}
