#pragma once

const int COLLISION_LAYERS = 32;

class Collidable
{
private:
	typedef std::list<Collidable *> List;
	static List sLayer[COLLISION_LAYERS];

	// layer collision masks
	static unsigned int sLayerMask[COLLISION_LAYERS];

	// list entry
	List::iterator entry;

	// collision layer
	int layer;

public:
	// bounding box
	AlignedBox2 box;

	enum Type
	{
		TYPE_NONE,
		TYPE_ALIGNED_BOX,
		TYPE_CIRCLE,
		NUM_TYPES
	};

	// collision type
	Type type;

	// size
	Vector2 size;

public:
	Collidable(int aLayer = -1);
	~Collidable(void);

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

	// control
	static void CollideAll(float aStep);
	virtual void Collide(float aStep, Collidable &aRecipient) = 0;
};
