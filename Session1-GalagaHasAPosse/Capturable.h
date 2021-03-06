#pragma once

#include "Database.h"

class CapturableTemplate
{
public:
	float mResistance;
	unsigned int mSpawnOnCapture;
	unsigned int mSwitchOnCapture;

public:
	CapturableTemplate(void);
	~CapturableTemplate(void);

	bool Configure(const tinyxml2::XMLElement *element);
};

class Capturable
{
protected:
	unsigned int mId;

	float mResistance;

public:
	typedef Signal<void (unsigned int, unsigned int)> CaptureSignal;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Capturable(void);
	Capturable(const CapturableTemplate &aTemplate, unsigned int aId);
	virtual ~Capturable(void);

	void Persuade(unsigned int aSourceId, float aEffect);

	void Capture(void);

	float GetResistance(void)
	{
		return mResistance;
	}
};

namespace Database
{
	extern Typed<CapturableTemplate> capturabletemplate;
	extern Typed<Capturable *> capturable;
	extern Typed<Typed<Capturable::CaptureSignal> > capturesignal;
}
