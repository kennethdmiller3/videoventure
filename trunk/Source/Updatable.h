#pragma once

class Updatable
{
protected:
	// identifier
	unsigned int id;

private:
	// linked list
	Updatable *mNext;
	Updatable *mPrev;

	// list entry
	typedef fastdelegate::FastDelegate<void (float)> Entry;
	Entry entry;

public:
	Updatable(unsigned int aId);
	virtual ~Updatable(void);

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

	// update
	static void UpdateAll(float aStep);
	virtual void Update(float aStep) = 0;
};
