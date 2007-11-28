#pragma once

class Simulatable
{
private:
	// list of all simulatables
	typedef std::list<Simulatable *> List;
	static List sAll;

	// list entry
	List::iterator entry;

public:
	Simulatable(void);
	~Simulatable(void);

	// simulate
	static void SimulateAll(float aStep);
	virtual void Simulate(float aStep) = 0;
};
