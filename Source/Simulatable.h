#pragma once

class Simulatable
{
private:
	// list of all simulatables
	typedef fastdelegate::FastDelegate1<float> Entry;
	typedef std::list<Entry> List;
	static List sAll;

	// list entry
	List::iterator entry;

protected:
	// identifier
	unsigned int id;

public:
	Simulatable(unsigned int aId);
	virtual ~Simulatable(void);

	// configure
	virtual bool Configure(TiXmlElement *element) { return false; }

	// simulate
	static void SimulateAll(float aStep);
	virtual void Simulate(float aStep) = 0;
};
