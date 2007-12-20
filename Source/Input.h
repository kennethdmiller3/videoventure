#pragma once

class Input
{
public:
	// logical inputs
	enum LOGICAL
	{
		MOVE_VERTICAL,
		MOVE_HORIZONTAL,
		AIM_VERTICAL,
		AIM_HORIZONTAL,
		FIRE_PRIMARY,
		NUM_LOGICAL
	};
	float value[NUM_LOGICAL];

	// input binding map
	struct Binding
	{
		LOGICAL target;	// target logical value
		float deadzone;	// deadzone threshold
		float scale;	// scale factor
		float previous;	// previous axis value
		bool pressed;	// pressed this turn
		bool released;	// released this turn
	};
	typedef std::multimap<int, Binding> Map;
	Map map;

public:
	Input(void);
	~Input(void);

	// add an input binding
	void Bind(LOGICAL aLogical, int aType, int aDevice, int aControl, float aDeadzone, float aScale);

	// update inputs
	void Update(void);

	// key events
	void OnAxis(int aType, int aDevice, int aControl, float aValue);
	void OnPress(int aType, int aDevice, int aControl);
	void OnRelease(int aType, int aDevice, int aControl);

	// get logical input
	inline float operator[](LOGICAL aLogical) const
	{
		return std::min(std::max(value[aLogical], -1.0f), 1.0f);
	}
};
