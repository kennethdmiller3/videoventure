#pragma once

#include "Database.h"
#include <boost/pool/pool.hpp>

const int COLLISION_LAYERS = 32;

class CollidableTemplate
{
public:
	// collision layer
	int layer;

	// collision shapes
	std::vector<b2ShapeDef *> shapes;

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
	virtual bool Configure(TiXmlElement *element);
};

class Collidable : public CollidableTemplate
{
protected:
	static b2World *world;

	// identifier
	unsigned int id;

	// body
	b2Body *body;

public:
	Collidable(void);
	Collidable(const CollidableTemplate &aTemplate);
	virtual ~Collidable(void);

	void AddToWorld(void);
	void RemoveFromWorld(void);

	b2Body *GetBody(void) const
	{
		return body;
	}

	// collision world
	static void Init(void);
	static void Done(void);

	// control
	static void CollideAll(float aStep);
	virtual void Collide(float aStep, Collidable &aRecipient)
	{
	};
};

namespace Database
{
	extern Typed<CollidableTemplate> collidabletemplate;
	extern Typed<Collidable> collidable;
}
