#pragma once

#include "Database.h"

const int COLLISION_LAYERS = 32;

class CollidableTemplate
{
public:
	// collision layer
	int layer;

	// collision shapes
	std::list<b2CircleDef> circles;
	std::list<b2BoxDef> boxes;
	std::list<b2PolyDef> polys;

	// collision bodies
	typedef stdext::hash_map<unsigned int, b2BodyDef> BodyMap;
	BodyMap bodies;

	// collision joints
	typedef stdext::hash_map<unsigned int, b2JointDef> JointMap;
	JointMap joints;

public:
	CollidableTemplate(void);
	virtual ~CollidableTemplate(void);

	// configure
	bool ProcessShapeItem(TiXmlElement *element, b2ShapeDef &shape);
	bool ConfigureShape(TiXmlElement *element, b2ShapeDef &shape);
	bool ConfigureCircle(TiXmlElement *element, b2CircleDef &shape);
	bool ConfigureBox(TiXmlElement *element, b2BoxDef &shape);
	bool ProcessPolyItem(TiXmlElement *element, b2PolyDef &shape);
	bool ConfigurePoly(TiXmlElement *element, b2PolyDef &shape);
	bool ProcessBodyItem(TiXmlElement *element, b2BodyDef &body);
	bool ConfigureBody(TiXmlElement *element, b2BodyDef &body);
	bool ProcessJointItem(TiXmlElement *element, b2JointDef &joint);
	bool ProcessRevoluteJointItem(TiXmlElement *element, b2RevoluteJointDef &joint);
	bool ConfigureRevoluteJoint(TiXmlElement *element, b2RevoluteJointDef &joint);
	bool ProcessPrismaticJointItem(TiXmlElement *element, b2PrismaticJointDef &joint);
	bool ConfigurePrismaticJoint(TiXmlElement *element, b2PrismaticJointDef &joint);
	bool ProcessDistanceJointItem(TiXmlElement *element, b2DistanceJointDef &joint);
	bool ConfigureDistanceJoint(TiXmlElement *element, b2DistanceJointDef &joint);
	bool ProcessPulleyJointItem(TiXmlElement *element, b2PulleyJointDef &joint);
	bool ConfigurePulleyJoint(TiXmlElement *element, b2PulleyJointDef &joint);
	bool ProcessMouseJointItem(TiXmlElement *element, b2MouseJointDef &joint);
	bool ConfigureMouseJoint(TiXmlElement *element, b2MouseJointDef &joint);
	virtual bool Configure(TiXmlElement *element);
};

class Collidable
{
protected:
	static b2World *world;

	// identifier
	unsigned int id;

	// body
	b2Body *body;

public:
	class Listener
	{
	public:
		virtual void Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount) = 0;
	};

	// listeners
	typedef stdext::hash_map<unsigned int, Listener *> ListenerMap;
	ListenerMap listeners;

public:
	Collidable(void);
	Collidable(const CollidableTemplate &aTemplate, unsigned int aId);
	virtual ~Collidable(void);

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
	virtual void Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount);
};

namespace Database
{
	extern Typed<CollidableTemplate> collidabletemplate;
	extern Typed<Collidable *> collidable;
}