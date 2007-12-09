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
	struct ColorLinear { float time; Color start; Color rate; };
	typedef std::vector<ColorLinear> ColorKeys;
	struct Scale { float x; float y; float z; };
	struct ScaleLinear { float time; Scale start; Scale rate; };
	typedef std::vector<ScaleLinear> ScaleKeys;
	ColorKeys mCoreColor;
	ScaleKeys mCoreScale;
	ColorKeys mHaloColor;
	ScaleKeys mHaloScale;

public:
	ExplosionTemplate(void);
	~ExplosionTemplate(void);

	// configure
	bool ConfigureScaleLinear(TiXmlElement *element, ExplosionTemplate::ScaleLinear &scale);
	bool ConfigureScale(TiXmlElement *element, ScaleKeys &scalekeys);
	bool ConfigureColorLinear(TiXmlElement *element, ExplosionTemplate::ColorLinear &color);
	bool ConfigureColor(TiXmlElement *element, ColorKeys &colorkeys);
	bool Configure(TiXmlElement *element);
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