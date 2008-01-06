#pragma once

#include "Entity.h"
#include "Simulatable.h"
#include "Renderable.h"

class ExplosionTemplate
{
public:
	// life span
	float mLifeSpan;

	// drawlist buffer
	std::vector<unsigned int> mBuffer;

public:
	ExplosionTemplate(void);
	~ExplosionTemplate(void);

	// configure
	bool Configure(TiXmlElement *element, unsigned int id);
};

class Explosion :
	public Simulatable, public Renderable
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// life
	float mLife;

public:
	Explosion(void);
	Explosion(const ExplosionTemplate &aTemplate, unsigned int aId);
	~Explosion(void);

	// simulate
	virtual void Simulate(float aStep);

	// render
	virtual void Render(const Matrix2 &transform);
};

namespace Database
{
	extern Typed<ExplosionTemplate> explosiontemplate;
	extern Typed<Explosion *> explosion;
}