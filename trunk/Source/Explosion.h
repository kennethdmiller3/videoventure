#pragma once

#include "Entity.h"
#include "Simulatable.h"
#include "Renderable.h"

#include <boost/pool/object_pool.hpp>

class Explosion :
	public Entity, public Simulatable, public Renderable
{
public:
	// explosion pool
	static boost::object_pool<Explosion> pool;

	// life
	float mLife;

public:
	Explosion(unsigned int aId = 0, unsigned int aParentId = 0);
	~Explosion(void);

	// simulate
	virtual void Simulate(float aStep);

	// render
	virtual void Render(const Matrix2 &transform);
};
