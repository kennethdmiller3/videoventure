#pragma once
#include "controller.h"

// aimer template
class AimerTemplate
{
public:
	// target scan
	float mPeriod;
	float mRange;
	float mFocus;
	unsigned short mCategoryBits;
	unsigned short mMaskBits;

	// attack cone
	float mAttack[2];
	float mAngle[2];

	// leading
	float mLeading;

	// evasion
	float mEvade;

	// close range
	float mClose;
	float mCloseDistScale;
	float mCloseSpeedScale;

	// far range
	float mFar;
	float mFarDistScale;
	float mFarSpeedScale;

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
	Vector2 mOffset;
	float mDelay;
	float mAvoidDelay;
	float mAvoidPriority;
	float mAvoidTurn;
	Vector2 mAvoidMove;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Aimer(const AimerTemplate &aTemplate, unsigned int aId = 0);
	virtual ~Aimer(void);

	// control
	void Control(float aStep);

protected:
	Vector2 LeadTarget(float bulletSpeed, const Vector2 &targetPosition, const Vector2 &targetVelocity);
};

namespace Database
{
	extern Typed<AimerTemplate> aimertemplate;
	extern Typed<Aimer *> aimer;
}