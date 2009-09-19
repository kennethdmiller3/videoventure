#include "StdAfx.h"
#include "Input.h"

// input names
static const char *logicalname[Input::NUM_LOGICAL] =
{
	"move_x",	// MOVE_HORIZONTAL,
	"move_y",	// MOVE_VERTICAL,
	"aim_x",	// AIM_HORIZONTAL,
	"aim_y",	// AIM_VERTICAL,
	"fire1",	// FIRE_PRIMARY,
	"fire2",	// FIRE_SECONDARY,
	"fire3",	// FIRE_CHANNEL3,
	"fire4",	// FIRE_CHANNEL4,
	"menu_x",	// MENU_HORIZONTAL
	"menu_y",	// MENU_VERTICAL
	"click",	// MENU_CLICK
};


Input::Input(void)
{
	Clear();
}

Input::~Input(void)
{
}

// clear input bindings
void Input::Clear(void)
{
	bindingmap.Clear();
	memset(value, 0, sizeof(value));
}

// add an input binding
void Input::Bind(LOGICAL aLogical, int aType, int aDevice, int aControl, float aDeadzone, float aScale, float aMin, float aMax)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	Bindings &bindings = bindingmap.Open(aPhysical);
	Binding &binding = bindings.Open(aLogical);
	binding.target = aLogical;
	binding.deadzone = aDeadzone;
	binding.scale = aScale;
	binding.min = aMin;
	binding.max = aMax;
	binding.previous = 0.0f;
	binding.pressed = false;
	binding.released = false;
	bindings.Close(aLogical);
	bindingmap.Close(aPhysical);
}

// update inputs
void Input::Update(void)
{
#ifdef INPUT_UNIT_VECTOR
	float scale;
	
	// limit magnitude of move control to 1
	scale = value[MOVE_VERTICAL]*value[MOVE_VERTICAL]+value[MOVE_HORIZONTAL]*value[MOVE_HORIZONTAL];
	if (scale > 1.0f)
		scale = InvSqrt(scale);
	else
		scale = 1.0f;
	output[MOVE_VERTICAL] = value[MOVE_VERTICAL] * scale;
	output[MOVE_HORIZONTAL] = value[MOVE_HORIZONTAL] * scale;
#else
	output[MOVE_VERTICAL] = Clamp(value[MOVE_VERTICAL], -1.0f, 1.0f);
	output[MOVE_HORIZONTAL] = Clamp(value[MOVE_HORIZONTAL], -1.0f, 1.0f);
#endif

#ifdef INPUT_UNIT_VECTOR
	// limit magnitude of aim control to 1
	scale = value[AIM_VERTICAL]*value[AIM_VERTICAL]+value[AIM_HORIZONTAL]*value[AIM_HORIZONTAL];
	if (scale > 1.0f)
		scale = InvSqrt(scale);
	else
		scale = 1.0f;
	output[AIM_VERTICAL] = value[AIM_VERTICAL] * scale;
	output[AIM_HORIZONTAL] = value[AIM_HORIZONTAL] * scale;
#elif 1
	output[AIM_VERTICAL] = Clamp(value[AIM_VERTICAL], -1.0f, 1.0f);
	output[AIM_HORIZONTAL] = Clamp(value[AIM_HORIZONTAL], -1.0f, 1.0f);
#else
	output[AIM_VERTICAL] = value[AIM_VERTICAL];
	output[AIM_HORIZONTAL] = value[AIM_HORIZONTAL];
#endif

	// limit magnitude of fire control to 1
	output[FIRE_PRIMARY] = Clamp(value[FIRE_PRIMARY], -1.0f, 1.0f);
	output[FIRE_SECONDARY] = Clamp(value[FIRE_SECONDARY], -1.0f, 1.0f);
	output[FIRE_CHANNEL3] = Clamp(value[FIRE_CHANNEL3], -1.0f, 1.0f);
	output[FIRE_CHANNEL4] = Clamp(value[FIRE_CHANNEL4], -1.0f, 1.0f);

	// limit magnitude of menu controls to 1
	output[MENU_HORIZONTAL] = Clamp(value[MENU_HORIZONTAL], -1.0f, 1.0f);
	output[MENU_VERTICAL] = Clamp(value[MENU_VERTICAL], -1.0f, 1.0f);
	output[MENU_CLICK] = Clamp(value[MENU_CLICK], -1.0f, 1.0f);
}

void Input::Playback(const TiXmlElement *element)
{
	// update the control values
	for (int i = 0; i < Input::NUM_LOGICAL; ++i)
		element->QueryIntAttribute(logicalname[i], reinterpret_cast<int *>(&output[i]));
}

void Input::Record(TiXmlElement *element, float prev[])
{
	// add changed control values
	for (int i = 0; i < Input::NUM_LOGICAL; ++i)
	{
		if (output[i] != prev[i])
			element->SetAttribute(logicalname[i], *reinterpret_cast<int *>(&output[i]));
	}
}

// step inputs
void Input::Step(void)
{
	for (Database::Typed<Bindings >::Iterator inputitor(&bindingmap); inputitor.IsValid(); ++inputitor)
	{
		for (Bindings::Iterator bindingitor(&inputitor.GetValue()); bindingitor.IsValid(); ++bindingitor)
		{
			Binding &binding = const_cast<Binding &>(bindingitor.GetValue());
			if (binding.released)
			{
				if (binding.pressed)
				{
					value[binding.target] -= binding.scale;
					binding.pressed = false;
				}
				binding.released = false;
			}
		}
	}
}

// on axis movement
void Input::OnAxis(int aType, int aDevice, int aControl, float aValue)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	for (Bindings::Iterator bindingitor(bindingmap.Find(aPhysical)); bindingitor.IsValid(); ++bindingitor)
	{
		Binding &binding = const_cast<Binding &>(bindingitor.GetValue());
		float scaled = aValue;
		if (scaled < -binding.deadzone)
			scaled = (scaled + binding.deadzone) * binding.scale;
		else if (scaled > binding.deadzone)
			scaled = (scaled - binding.deadzone) * binding.scale;
		else
			scaled = 0.0f;
		scaled = Clamp(scaled, binding.min, binding.max);
		value[binding.target] += scaled - binding.previous;
		binding.previous = scaled;
#ifdef PRINT_AXIS_UPDATE
		DebugPrint("target=%d value=%f scaled=%f logical=%f\n", binding.target, aValue, scaled, value[binding.target]);
#endif
	}
}

// on button press
void Input::OnPress(int aType, int aDevice, int aControl)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	for (Bindings::Iterator bindingitor(bindingmap.Find(aPhysical)); bindingitor.IsValid(); ++bindingitor)
	{
		Binding &binding = const_cast<Binding &>(bindingitor.GetValue());
		if (!binding.pressed)
		{
			value[binding.target] += binding.scale;
			binding.pressed = true;
		}
	}
}

// on button release
void Input::OnRelease(int aType, int aDevice, int aControl)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	for (Bindings::Iterator bindingitor(bindingmap.Find(aPhysical)); bindingitor.IsValid(); ++bindingitor)
	{
		Binding &binding = const_cast<Binding &>(bindingitor.GetValue());
		binding.released = true;
	}
}

// configure an item
void Input::ConfigureItem(const TiXmlElement *element)
{
	const char *value = element->Value();
	switch (Hash(value))
	{
	case 0xc7535f2e /* "bind" */:
		{
			// map logical name
			const char *name = element->Attribute("name");
			LOGICAL logical = NUM_LOGICAL;
			for (int i = 0; i < NUM_LOGICAL; ++i)
			{
				if (Hash(name) == Hash(logicalname[i]))
				{
					logical = LOGICAL(i);
					break;
				}
			}

			// map input type
			const char *type = element->Attribute("type");
			TYPE inputtype;
			switch(Hash(type))
			{
			case 0x4aa845f4 /* "keyboard" */:			inputtype = TYPE_KEYBOARD; break;
			case 0xd76afdc0 /* "mouse_axis" */:			inputtype = TYPE_MOUSE_AXIS; break;
			case 0xbe730575 /* "mouse_button" */:		inputtype = TYPE_MOUSE_BUTTON; break;
			case 0x4b1fb051 /* "joystick_axis" */:		inputtype = TYPE_JOYSTICK_AXIS; break;
			case 0xb084d264 /* "joystick_button" */:	inputtype = TYPE_JOYSTICK_BUTTON; break;
			default:									inputtype = NUM_TYPES; break;
			}

			// get properties
			int device = 0;
			element->QueryIntAttribute("device", &device);
			int control = 0;
			element->QueryIntAttribute("control", &control);
			float deadzone = 0.0f;
			element->QueryFloatAttribute("deadzone", &deadzone);
			float scale = 1.0f;
			element->QueryFloatAttribute("scale", &scale);
			float minimum = -FLT_MAX;
			element->QueryFloatAttribute("min", &minimum);
			float maximum = FLT_MAX;
			element->QueryFloatAttribute("max", &maximum);

			Bind(logical, inputtype, device, control, deadzone, scale, minimum, maximum);
		}
		break;
	}
}

// configure
void Input::Configure(const TiXmlElement *element)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureItem(child);
	}
}
