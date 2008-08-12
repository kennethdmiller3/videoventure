#pragma once

#include "Entity.h"
#include "Updatable.h"
#include "Renderable.h"

class CaptureTemplate
{
public:
	// energy
	float mEnergy;
	float mConsume;
	float mRecover;

	// field
	float mStrength;
	float mRadius;
	float mAngle;

	// fire channel
	int mChannel;

public:
	CaptureTemplate(void);
	~CaptureTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element, unsigned int id);
};

class Capture :
	public Updatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	// controller
	unsigned int mControlId;

	// fire channel
	int mChannel;

	// energy
	float mEnergy;

	// active
	bool mActive;

public:
	Capture(void);
	Capture(const CaptureTemplate &aTemplate, unsigned int aId);
	~Capture(void);

	// set control
	void SetControl(unsigned int aControlId)
	{
		mControlId = aControlId;
	}

	// simulate
	void Update(float aStep);

protected:
	// start
	void Start(void);

	// stop
	void Stop(void);
};

namespace Database
{
	extern Typed<CaptureTemplate> capturetemplate;
	extern Typed<Capture *> capture;
}