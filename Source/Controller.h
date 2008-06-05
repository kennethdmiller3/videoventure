#pragma once

class Controller
{
protected:
	// identifier
	unsigned int mId;

private:
	// linked list
	Controller *mNext;
	Controller *mPrev;

	// list entry
	typedef fastdelegate::FastDelegate<void (float)> Entry;
	Entry entry;

public:
	static const int FIRE_CHANNELS = 2;

	// controls
	Vector2 mMove;
	Vector2 mAim;
	bool mFire[FIRE_CHANNELS];

public:
	Controller(unsigned int aId);
	virtual ~Controller(void);

	// activate
	void Activate(void);
	void Deactivate(void);

	// is active?
	bool IsActive(void)
	{
		return !entry.empty();
	}

	// configure
	virtual bool Configure(const TiXmlElement *element) { return false; }

	// control
	static void ControlAll(float aStep);
	virtual void Control(float aStep) = 0;
};

namespace Database
{
	extern Typed<Controller *> controller;
}
