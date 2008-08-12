#include "StdAfx.h"
#include "Input.h"

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
void Input::Bind(LOGICAL aLogical, int aType, int aDevice, int aControl, float aDeadzone, float aScale)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	Bindings &bindings = bindingmap.Open(aPhysical);
	Binding &binding = bindings.Open(aLogical);
	binding.target = aLogical;
	binding.deadzone = aDeadzone;
	binding.scale = aScale;
	binding.previous = 0.0f;
	binding.pressed = false;
	binding.released = false;
	bindings.Close(aLogical);
	bindingmap.Close(aPhysical);
}

// update inputs
void Input::Update(void)
{
	float scale;
	
	// limit magnitude of move control to 1
	scale = value[MOVE_VERTICAL]*value[MOVE_VERTICAL]+value[MOVE_HORIZONTAL]*value[MOVE_HORIZONTAL];
	if (scale > 1.0f)
		scale = InvSqrt(scale);
	else
		scale = 1.0f;
	output[MOVE_VERTICAL] = value[MOVE_VERTICAL] * scale;
	output[MOVE_HORIZONTAL] = value[MOVE_HORIZONTAL] * scale;

	// limit magnitude of aim control to 1
	scale = value[AIM_VERTICAL]*value[AIM_VERTICAL]+value[AIM_HORIZONTAL]*value[AIM_HORIZONTAL];
	if (scale > 1.0f)
		scale = InvSqrt(scale);
	else
		scale = 1.0f;
	output[AIM_VERTICAL] = value[AIM_VERTICAL] * scale;
	output[AIM_HORIZONTAL] = value[AIM_HORIZONTAL] * scale;

	// limit magnitude of fire control to 1
	output[FIRE_PRIMARY] = std::min(std::max(value[FIRE_PRIMARY], -1.0f), 1.0f);
	output[FIRE_SECONDARY] = std::min(std::max(value[FIRE_SECONDARY], -1.0f), 1.0f);
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

// process a configuration item
void Input::ProcessItem(const TiXmlElement *element)
{
	const char *value = element->Value();
	switch (Hash(value))
	{
	case 0xc7535f2e /* "bind" */:
		{
			// map logical name
			const char *name = element->Attribute("name");
			LOGICAL logical;
			switch(Hash(name))
			{
			case 0x2f7d674b /* "move_x" */:	logical = MOVE_HORIZONTAL; break;
			case 0x2e7d65b8 /* "move_y" */:	logical = MOVE_VERTICAL; break;
			case 0x28e0ac09 /* "aim_x" */:	logical = AIM_HORIZONTAL; break;
			case 0x27e0aa76 /* "aim_y" */:	logical = AIM_VERTICAL; break;
			case 0x8eab16d9 /* "fire" */:
			case 0x7f550f38 /* "fire1" */:	logical = FIRE_PRIMARY; break;
			case 0x825513f1 /* "fire2" */:	logical = FIRE_SECONDARY; break;
			default:						logical = NUM_LOGICAL; break;
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

			input.Bind(logical, inputtype, device, control, deadzone, scale);
		}
		break;
	}
}

// configure
void Input::Configure(const TiXmlElement *element)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessItem(child);
	}
}
