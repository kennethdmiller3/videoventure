#pragma once

class GAME_API Simulatable
{
public:
	typedef fastdelegate::FastDelegate<void (float)> Action;

protected:
	// identifier
	unsigned int mId;

private:
	// linked list
	Simulatable *mNext;
	Simulatable *mPrev;
	bool mActive;

	// action
	Action mAction;

public:
	Simulatable(unsigned int aId);
	virtual ~Simulatable(void);

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

	// simulate
	static void SimulateAll(float aStep);
};
