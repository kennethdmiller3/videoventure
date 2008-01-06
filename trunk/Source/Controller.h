#pragma once

class Controller
{
private:
	// list of all controllables
	typedef fastdelegate::FastDelegate<void (float)> Entry;
	typedef std::list<Entry> List;
	static List sAll;

	// list entry
	List::iterator entry;

protected:
	// identifier
	unsigned int id;

public:
	// controls
	Vector2 mMove;
	Vector2 mAim;
	bool mFire;

public:
	Controller(unsigned int aId);
	virtual ~Controller(void);

	// configure
	virtual bool Configure(TiXmlElement *element) { return false; }

	// control
	static void ControlAll(float aStep);
	virtual void Control(float aStep) = 0;
};

namespace Database
{
	extern Typed<Controller *> controller;
}
