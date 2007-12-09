#pragma once

class Controllable
{
private:
	// list of all controllables
	typedef std::list<Controllable *> List;
	static List sAll;

	// list entry
	List::iterator entry;

protected:
	// identifier
	unsigned int id;

public:
	Controllable(unsigned int aId);
	virtual ~Controllable(void);

	// configure
	virtual bool Configure(TiXmlElement *element) { return false; }

	// control
	static void ControlAll(float aStep);
	virtual void Control(float aStep) = 0;
};
