#pragma once

#include "Database.h"

const int COLLISION_LAYERS = 32;

class CollidableTemplate
{
public:
	// collision layer
	int layer;

	// collision type
	enum Type
	{
		TYPE_NONE,
		TYPE_ALIGNED_BOX,
		TYPE_CIRCLE,
		NUM_TYPES
	};
	Type type;

	// size
	Vector2 size;

public:
	CollidableTemplate(void);
	virtual ~CollidableTemplate(void);

	// configure
	virtual bool Attribute(TiXmlAttribute *attribute);
	virtual bool Configure(TiXmlElement *element);
};

class Collidable : public CollidableTemplate
{
private:
	typedef std::list<Collidable *> List;
	static List sLayer[COLLISION_LAYERS];

	// layer collision masks
	static unsigned int sLayerMask[COLLISION_LAYERS];

	// identifier
	unsigned int id;

	// list entry
	List::iterator entry;

public:
	// bounding box
	AlignedBox2 box;

public:
	Collidable(void);
	Collidable(const CollidableTemplate &aTemplate);
	virtual ~Collidable(void);

	// get layer collision mask
	static unsigned int GetLayerMask(int aLayer)
	{
		if (aLayer >= 0 && aLayer < 32)
			return sLayerMask[aLayer];
		else
			return 0;
	}

	// set layer collision mask
	static void SetLayerMask(int aLayer, unsigned int aMask)
	{
		if (aLayer >= 0 && aLayer < 32)
			sLayerMask[aLayer] = aMask;
	}

	// get layer
	int GetLayer(void)
	{
		return layer;
	}

	// set layer
	void SetLayer(int aLayer);

	// configure
	virtual bool Attribute(TiXmlAttribute *attribute);

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
