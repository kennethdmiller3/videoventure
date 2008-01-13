#pragma once

#include "Simulatable.h"

// gunner template
class GunnerTemplate
{
public:
	float mFollowLength;

public:
	GunnerTemplate(void);
	~GunnerTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

// gunner actor
class Gunner : 
	public Simulatable
{
protected:
	std::deque<Vector2> mTrackPos;
	float mTrackLength;

public:
	// constructor
	Gunner(const GunnerTemplate &aTemplate, unsigned int aId);

	// destructor
	~Gunner(void);

	// simulate
	virtual void Simulate(float aStep);
};

namespace Database
{
	extern Typed<Gunner *> gunner;
}