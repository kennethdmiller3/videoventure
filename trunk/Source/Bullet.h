#pragma once

#include "Entity.h"
#include "Simulatable.h"
#include "Collidable.h"
#include "Renderable.h"

class BulletTemplate
{
public:
	// life span
	float mLife;

public:
	BulletTemplate(void);
	virtual ~BulletTemplate(void);

	// configure
	virtual bool Configure(TiXmlElement *element);
};

class Bullet
	: public Simulatable, Collidable::Listener
{
public:
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);

	// life timer
	float mLife;

public:
	Bullet(void);
	Bullet(const BulletTemplate &aTemplate, unsigned int aId);
	~Bullet(void);

	// simulate
	virtual void Simulate(float aStep);

	// collide
	virtual void Collide(Collidable &aRecipient, b2Manifold aManifold[], int aCount);
};

namespace Database
{
	extern Typed<BulletTemplate> bullettemplate;
	extern Typed<Bullet *> bullet;
}
