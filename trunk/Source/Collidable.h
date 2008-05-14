#pragma once

#include "Database.h"

const int COLLISION_LAYERS = 32;

class LinkTemplate;

class CollidableTemplate
{
public:
	// identifier
	unsigned int id;

	// body definition
	b2BodyDef bodydef;

#ifndef COLLIDABLE_SHAPE_DATABASE
	// shape definitions
	std::vector<b2ShapeDef *> shapes;
#endif

#ifndef COLLIDABLE_JOINT_DATABASE
	// joint definitions
	std::vector<b2JointDef *> joints;
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
	static void WorldInit(void);
	static void WorldDone(void);
	static b2World *GetWorld(void)
	{
		return world;
	}

	// control
	static void CollideAll(float aStep);

protected:
	bool CreateJoint(b2JointDef &joint) const;
};

namespace Database
{
	extern Typed<CollidableTemplate> collidabletemplate;
	extern Typed<Collidable *> collidable;
	extern Typed<Typed<Collidable::Listener> > collidablecontactadd;
	extern Typed<Typed<Collidable::Listener> > collidablecontactremove;
}
