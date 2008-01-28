#pragma once

class Controller
{
protected:
	// identifier
	unsigned int id;

private:
	// linked list
	Controller *mNext;
	Controller *mPrev;

	// list entry
	typedef fastdelegate::FastDelegate<void (float)> Entry;
	Entry entry;

public:
	// controls
	Vector2 mMove;
	Vector2 mAim;
	bool mFire[2];

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
