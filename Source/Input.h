#pragma once

class Input
{
public:
	// input types
	enum TYPE
	{
		TYPE_KEYBOARD,
		TYPE_MOUSE_AXIS,
		TYPE_MOUSE_BUTTON,
		TYPE_JOYSTICK_AXIS,
		TYPE_JOYSTICK_BUTTON,
		NUM_TYPES
	};

	// logical inputs
	enum LOGICAL
	{
		MOVE_VERTICAL,
		MOVE_HORIZONTAL,
		AIM_VERTICAL,
		AIM_HORIZONTAL,
		FIRE_PRIMARY,
		FIRE_SECONDARY,
		NUM_LOGICAL
	};
	float value[NUM_LOGICAL];
	float output[NUM_LOGICAL];

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
	typedef Database::Typed<Input::Binding> Bindings;
	Database::Typed<Bindings> bindingmap;

public:
	Input(void);
	~Input(void);

	// clear input bindings
	void Clear(void);

	// add an input binding
	void Bind(LOGICAL aLogical, int aType, int aDevice, int aControl, float aDeadzone, float aScale);

	// update inputs
	void Update(void);

	// step inputs
	void Step(void);

	// input events
	void OnAxis(int aType, int aDevice, int aControl, float aValue);
	void OnPress(int aType, int aDevice, int aControl);
	void OnRelease(int aType, int aDevice, int aControl);

	// get logical input
	inline float operator[](LOGICAL aLogical) const
	{
		return output[aLogical];
	}

	// configure input
	void ProcessItem(const TiXmlElement *element);
	void Configure(const TiXmlElement *element);
};
