#pragma once

class Input
{
public:
	// logical inputs
	enum LOGICAL
	{
		MOVE_UP,
		MOVE_DOWN,
		MOVE_LEFT,
		MOVE_RIGHT,
		FIRE_PRIMARY,
		NUM_LOGICAL
	};
	bool set[NUM_LOGICAL];
	bool clear[NUM_LOGICAL];

	// input map
	typedef stdext::hash_map<int, LOGICAL> Map;
	Map map;

public:
	Input(void);
	~Input(void);

	// add an input binding
	void Bind(LOGICAL aLogical, int aPhysical)
	{
		map[aPhysical] = aLogical;
	}

	// start input cycle
	void Start(void);

	// key events
	void OnKeyDown(int aKey);
	void OnKeyUp(int aKey);

	// get logical input
	inline bool operator[](LOGICAL aLogical) const
	{
		return set[aLogical];
	}
};
