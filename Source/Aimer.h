#pragma once
#include "controller.h"

// aimer template
class AimerTemplate
{
public:
	// target range
	float mRange;
	float mAttack;

	// direction focus
	float mFocus;

public:
	AimerTemplate(void);
	~AimerTemplate(void);

	// configure
	bool Configure(TiXmlElement *element);
};

// aimer controller
class Aimer :
	public Controller
{
protected:
	unsigned int mTarget;

public:
	Aimer(const AimerTemplate &aTemplate, unsigned int aId = 0);
	virtual ~Aimer(void);

	// control
	virtual void Control(float aStep);

protected:
	Vector2 LeadTarget(const Vector2 &startPosition, float bulletSpeed, const Vector2 &targetPosition, const Vector2 &targetVelocity);
};

namespace Database
{
	extern Typed<AimerTemplate> aimertemplate;
	extern Typed<Aimer *> aimer;
}