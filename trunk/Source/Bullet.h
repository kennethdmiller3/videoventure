#pragma once

#include "Entity.h"
#include "Simulatable.h"
#include "Collidable.h"
#include "Renderable.h"

#include <boost/pool/object_pool.hpp>

class Bullet :
	public Entity, public Simulatable, public Collidable, public Renderable
{
public:
	// bullet pool
	static boost::object_pool<Bullet> pool;

	// life
	float mLife;

public:
	Bullet(unsigned int aId = 0, unsigned int aParentId = 0);
	~Bullet(void);

	// simulate
	virtual void Simulate(float aStep);

	// collide
	virtual void Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount);

	// render
	virtual void Render(const Matrix2 &transform);
};
