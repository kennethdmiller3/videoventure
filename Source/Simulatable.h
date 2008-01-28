#pragma once

class Simulatable
{
protected:
	// identifier
	unsigned int id;

private:
	// linked list
	Simulatable *mNext;
	Simulatable *mPrev;

	// list entry
	typedef fastdelegate::FastDelegate<void (float)> Entry;
	Entry entry;

public:
	Simulatable(unsigned int aId);
	virtual ~Simulatable(void);

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

	// simulate
	static void SimulateAll(float aStep);
	virtual void Simulate(float aStep) = 0;
};
