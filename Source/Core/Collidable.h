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
	bool ConfigureEdge(const TiXmlElement *element, b2EdgeShape &shape);
	bool ConfigureChain(const TiXmlElement *element, b2ChainShape &shape);
	bool ConfigureBodyItem(const TiXmlElement *element, b2BodyDef &body, unsigned int id);
	bool ConfigureBody(const TiXmlElement *element, b2BodyDef &body, unsigned int id);
	bool Configure(const TiXmlElement *element, unsigned int id);
};

namespace Collidable
{
	typedef Signal<void (unsigned int id1, unsigned int id2, float t, const b2Contact &contact)> ContactSignal;

	// initialize the physics world
	void WorldInit(float aMinX, float aMinY, float aMaxX, float aMaxY, bool aWall);

	// clean up the physics world
	void WorldDone(void);

	// add an entity to the physics world
	GAME_API void AddToWorld(unsigned int aId);

	// remove an entity from the physics world
	GAME_API void RemoveFromWorld(unsigned int aId);

	// get the physics world
	GAME_API b2World *GetWorld(void);

	// get the world boundary
	GAME_API const b2AABB &GetBoundary(void);

	// default filter
	inline const b2Filter &GetDefaultFilter(void)
	{
		static b2Filter filter;
		return filter;
	}

	// check filtering
	inline bool CheckFilter(const b2Filter &aFilter1, const b2Filter &aFilter2)
	{
		if (aFilter1.groupIndex == aFilter2.groupIndex && aFilter1.groupIndex != 0)
			return aFilter1.groupIndex > 0;
		return 
			(aFilter1.maskBits & aFilter2.categoryBits) != 0 && 
			(aFilter1.categoryBits & aFilter2.maskBits) != 0;
	}

	// test segment for intersection with world shapes
	GAME_API unsigned int TestSegment(const b2Vec2 &aStart, const b2Vec2 &aEnd, const b2Filter &aFilter, unsigned int aId,
						   			  float &aLambda, b2Vec2 &aNormal, b2Fixture *&aShape);

	// wrapper for world query aabb
	GAME_API void QueryAABB(b2QueryCallback* callback, const b2AABB& aabb);

	// control
	void CollideAll(float aStep);
};

namespace Database
{
	extern GAME_API Typed<CollidableTemplate> collidabletemplate;
	extern GAME_API Typed<b2Body *> collidablebody;
	extern GAME_API Typed<Collidable::ContactSignal> collidablecontactadd;
	extern GAME_API Typed<Collidable::ContactSignal> collidablecontactremove;
}
