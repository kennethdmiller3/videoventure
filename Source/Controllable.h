#pragma once

class Controllable
{
private:
	// list of all controllables
	typedef std::list<Controllable *> List;
	static List sAll;

	// list entry
	List::iterator entry;

public:
	Controllable(void);
	~Controllable(void);

	// configure
	virtual bool Configure(TiXmlElement *element) { return false; }

	// control
	static void ControlAll(float aStep);
	virtual void Control(float aStep) = 0;
};
