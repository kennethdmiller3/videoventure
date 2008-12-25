#pragma once

class Updatable
{
public:
	typedef fastdelegate::FastDelegate<void (float)> Action;

protected:
	// identifier
	unsigned int mId;

private:
	// linked list
	Updatable *mNext;
	Updatable *mPrev;
	bool mActive;

	// action
	Action mAction;

public:
	Updatable(unsigned int aId);
	virtual ~Updatable(void);

	// set action
	void SetAction(Action aAction)
	{
		mAction = aAction;
	}

	// activate
	void Activate(void);
	void Deactivate(void);

	// get identifier
	unsigned int GetId() const
	{
		return mId;
	}

	// is active?
	bool IsActive(void)
	{
		return mActive;
	}

	// update
	static void UpdateAll(float aStep);
};
