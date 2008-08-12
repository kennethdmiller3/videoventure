#pragma once

class Controller
{
public:
	typedef fastdelegate::FastDelegate<void (float)> Action;

protected:
	// identifier
	unsigned int mId;

private:
	// linked list
	Controller *mNext;
	Controller *mPrev;
	bool mActive;

	// action
	Action mAction;

public:
	static const int FIRE_CHANNELS = 2;

	// controls
	Vector2 mMove;
	float mTurn;
	bool mFire[FIRE_CHANNELS];

public:
	Controller(unsigned int aId);
	virtual ~Controller(void);

	// set action
	void SetAction(Action aAction)
	{
		mAction = aAction;
	}

	// activate
	void Activate(void);
	void Deactivate(void);

	// is active?
	bool IsActive(void)
	{
		return mActive;
	}

	// configure
	virtual bool Configure(const TiXmlElement *element) { return false; }

	// control
	static void ControlAll(float aStep);
};

namespace Database
{
	extern Typed<Controller *> controller;
}
