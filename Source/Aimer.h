#pragma once
#include "controller.h"

// aimer template
class AimerTemplate
{
public:
	// target scan
	float mPeriod;
	float mRange;

	// attack cone
	float mAttack;
	float mAngle;

	// direction focus
	float mFocus;

	// leading
	float mLeading;

	// evasion
	float mEvade;

	// range
	float mClose;
	float mFar;

public:
	AimerTemplate(void);
	~AimerTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

// aimer controller
class Aimer :
	public Controller
{
protected:
	unsigned int mTarget;
	float mDelay;


public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Aimer(const AimerTemplate &aTemplate, unsigned int aId = 0);
	virtual ~Aimer(void);

	// control
	virtual void Control(float aStep);

protected:
	Vector2 LeadTarget(float bulletSpeed, const Vector2 &targetPosition, const Vector2 &targetVelocity);
};

namespace Database
{
	extern Typed<AimerTemplate> aimertemplate;
	extern Typed<Aimer *> aimer;
}