#pragma once

#include "Database.h"

const int COLLISION_LAYERS = 32;

class CollidableTemplate
{
public:
	// identifier
	unsigned int id;
	// collision layer
	int layer;


	// collision joints
	struct JointTemplate { unsigned int name1; unsigned int body1; unsigned int name2; unsigned int body2; };
	std::list<JointTemplate> joints;
	std::list<b2RevoluteJointDef> revolutes;
	std::list<b2PrismaticJointDef> prismatics;
	std::list<b2DistanceJointDef> distances;
	std::list<b2PulleyJointDef> pulleys;
	std::list<b2MouseJointDef> mouses;

public:
	CollidableTemplate(void);
	~CollidableTemplate(void);

	// configure
	bool ProcessShapeItem(const TiXmlElement *element, b2ShapeDef &shape);
	bool ConfigureShape(const TiXmlElement *element, b2ShapeDef &shape);
	bool ConfigureCircle(const TiXmlElement *element, b2CircleDef &shape);
	bool ConfigureBox(const TiXmlElement *element, b2PolygonDef &shape);
	bool ProcessPolyItem(const TiXmlElement *element, b2PolygonDef &shape);
	bool ConfigurePoly(const TiXmlElement *element, b2PolygonDef &shape);
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
};

class Collidable
{
protected:
	static b2World *world;

	// identifier
	unsigned int id;

	// primary body
	b2Body *body;

protected:
	bool SetupJointDef(b2JointDef &joint);

public:
	typedef fastdelegate::FastDelegate<void (unsigned int, float, const b2Manifold[], int)> Listener;

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
	static void WorldInit(void);
	static void WorldDone(void);
	static b2World *GetWorld(void)
	{
		return world;
	}

	// control
	static void CollideAll(float aStep);
};

namespace Database
{
	extern Typed<CollidableTemplate> collidabletemplate;
	extern Typed<Typed<b2BodyDef> > collidabletemplatebody;
	extern Typed<Collidable *> collidable;
	extern Typed<Typed<b2Body *> > collidablebody;
	extern Typed<Typed<Collidable::Listener> > collidablelistener;
}
