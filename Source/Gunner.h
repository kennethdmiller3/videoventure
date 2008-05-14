#pragma once

#include "Simulatable.h"

//#define GUNNER_TRACK_DEQUE

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
#ifdef GUNNER_TRACK_DEQUE
	std::deque<Vector2> mTrackPos;
#else
	Vector2 *mTrackPos;
	size_t mTrackCount;
	size_t mTrackFirst;
	size_t mTrackLast;
#endif
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