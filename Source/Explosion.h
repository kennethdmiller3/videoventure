#pragma once

#include "Entity.h"
#include "Simulatable.h"
#include "Renderable.h"
#include <map>

class ExplosionTemplate
{
public:
	// life span
	float mLifeSpan;

	// explosion keyframes
	struct Color { float r; float g; float b; float a; };
	typedef std::map<float, Color> ColorKeys;
	struct Scale { float x; float y; float z; };
	typedef std::map<float, Scale> ScaleKeys;
	ColorKeys mCoreColor;
	ScaleKeys mCoreScale;
	ColorKeys mHaloColor;
	ScaleKeys mHaloScale;

public:
	ExplosionTemplate(void);
	virtual ~ExplosionTemplate(void);

	// configure
	bool ConfigureScale(TiXmlElement *element, ScaleKeys &scalekeys);
	bool ConfigureColor(TiXmlElement *element, ColorKeys &colorkeys);
	virtual bool Configure(TiXmlElement *element);
};

class Explosion :
	public ExplosionTemplate, Simulatable, public Renderable
{
public:
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);

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