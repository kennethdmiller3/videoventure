#include "StdAfx.h"
#include "Input.h"

// input names
static const char * const logicalname[Input::NUM_LOGICAL] =
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

// TO DO: migrate this to platform
#if defined(USE_SDL)
static const char * const physicalname_key[SDLK_LAST] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"backspace", "tab", NULL, NULL, "clear", "enter", NULL, NULL,
	NULL, NULL, NULL, "pause", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "escape", NULL, NULL, NULL, NULL,
	"space", "\x21", "\x22", "\x23", "\x24", "\x25", "\x26", "\x27",
	"\x28", "\x29", "\x2A", "\x2B", "\x2C", "\x2D", "\x2E", "\x2F",
	"\x30", "\x31", "\x32", "\x33", "\x34", "\x35", "\x36", "\x37",
	"\x38", "\x39", "\x3A", "\x3B", "\x3C", "\x3D", "\x3E", "\x3F",
	"\x40", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "\x5B", "\x5C", "\x5D", "\x5E", "\x5F",
	"\x60", "\x61", "\x62", "\x63", "\x64", "\x65", "\x66", "\x67",
	"\x68", "\x69", "\x6A", "\x6B", "\x6C", "\x6D", "\x6E", "\x6F",
	"\x70", "\x71", "\x72", "\x73", "\x74", "\x75", "\x76", "\x77",
	"\x78", "\x79", "\x7A", "\x7B", "\x7C", "\x7D", "\x7E", "del",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"\xA0", "\xA1", "\xA2", "\xA3", "\xA4", "\xA5", "\xA6", "\xA7",
	"\xA8", "\xA9", "\xAA", "\xAB", "\xAC", "\xAD", "\xAE", "\xAF",
	"\xB0", "\xB1", "\xB2", "\xB3", "\xB4", "\xB5", "\xB6", "\xB7",
	"\xB8", "\xB9", "\xBA", "\xBB", "\xBC", "\xBD", "\xBE", "\xBF",
	"\xC0", "\xC1", "\xC2", "\xC3", "\xC4", "\xC5", "\xC6", "\xC7",
	"\xC8", "\xC9", "\xCA", "\xCB", "\xCC", "\xCD", "\xCE", "\xCF",
	"\xD0", "\xD1", "\xD2", "\xD3", "\xD4", "\xD5", "\xD6", "\xD7",
	"\xD8", "\xD9", "\xDA", "\xDB", "\xDC", "\xDD", "\xDE", "\xDF",
	"\xE0", "\xE1", "\xE2", "\xE3", "\xE4", "\xE5", "\xE6", "\xE7",
	"\xE8", "\xE9", "\xEA", "\xEB", "\xEC", "\xED", "\xEE", "\xEF",
	"\xF0", "\xF1", "\xF2", "\xF3", "\xF4", "\xF5", "\xF6", "\xF7",
	"\xF8", "\xF9", "\xFA", "\xFB", "\xFC", "\xFD", "\xFE", "\xFF",
	"keypad0", "keypad1", "keypad2", "keypad3", "keypad4", "keypad5", "keypad6", "keypad7",
	"keypad8", "keypad9", "keypaddecimal", "keypaddivide", "keypadmultiply", "keypadsubtract", "keypadadd", "keypadenter",
	"keypadequal", "up", "down", "right", "left", "insert", "home", "end",
	"pageup", "pagedown", "f1", "f2", "f3", "f4", "f5", "f6",
	"f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14",
	"f15", NULL, NULL, NULL, "numlock", "capslock", "scrolllock", "rshift",
	"lshift", "rctrl", "lctrl", "ralt", "lalt", "rmeta", "lmeta", "lsuper",
	"rsuper", "mode", "compose", "help", "print", "sysreq", "break", "menu",
	"power", "euro", "undo"
};
static const char * const physicalname_mouseaxis[] =
{
	"x", "y"
};
static const char * const physicalname_mousebutton[] =
{
	NULL, "left", "middle", "right", "wheelup", "wheeldown", "x1", "x2"
};
#elif defined(USE_GLFW)
static const char * const physicalname_key[GLFW_KEY_LAST] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"space", "\x21", "\x22", "\x23", "\x24", "\x25", "\x26", "\x27",
	"\x28", "\x29", "\x2A", "\x2B", "\x2C", "\x2D", "\x2E", "\x2F",
	"\x30", "\x31", "\x32", "\x33", "\x34", "\x35", "\x36", "\x37",
	"\x38", "\x39", "\x3A", "\x3B", "\x3C", "\x3D", "\x3E", "\x3F",
	"\x40", "\x41", "\x42", "\x43", "\x44", "\x45", "\x46", "\x47",
	"\x48", "\x49", "\x4A", "\x4B", "\x4C", "\x4D", "\x4E", "\x4F",
	"\x50", "\x51", "\x52", "\x53", "\x54", "\x55", "\x56", "\x57",
	"\x58", "\x59", "\x5A", "\x5B", "\x5C", "\x5D", "\x5E", "\x5F",
	"\x60", "\x61", "\x62", "\x63", "\x64", "\x65", "\x66", "\x67",
	"\x68", "\x69", "\x6A", "\x6B", "\x6C", "\x6D", "\x6E", "\x6F",
	"\x70", "\x71", "\x72", "\x73", "\x74", "\x75", "\x76", "\x77",
	"\x78", "\x79", "\x7A", "\x7B", "\x7C", "\x7D", "\x7E", "del",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, "escape", "f1", "f2", "f3", "f4", "f5", "f6",
	"f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14",
	"f15", "f16", "f17", "f18", "F19", "f20", "f21", "f22",
	"f23", "f24", "f25", "up", "down", "left", "right", "lshift",
	"rshift", "lctrl", "rctrl", "lalt", "ralt", "tab", "enter", "backspace",
	"insert", "delete", "pageup", "pagedown", "home", "end", "keypad0",
	"keypad1", "keypad2", "keypad3", "keypad4", "keypad5", "keypad6", "keypad7", "keypad8",
	"keypad9", "keypaddivide", "keypadmultiply", "keypadsubtract", "keypadadd", "keypaddecimal", "keypadequal", "keypadenter"
};
static const char * const physicalname_mouseaxis[] =
{
	"x", "y"
};
static const char * const physicalname_mousebutton[] =
{
	"left", "right", "middle"
};
#endif

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

void Input::Playback(const tinyxml2::XMLElement *element)
{
	// update the control values
	for (int i = 0; i < Input::NUM_LOGICAL; ++i)
		element->QueryIntAttribute(logicalname[i], reinterpret_cast<int *>(&output[i]));
}

void Input::Record(tinyxml2::XMLElement *element, float prev[])
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
void Input::ConfigureItem(const tinyxml2::XMLElement *element)
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
			const char * const *physicalname = NULL;
			int physicalnamecount = 0;
			switch(Hash(type))
			{
			case 0x4aa845f4 /* "keyboard" */:
				inputtype = TYPE_KEYBOARD;
				physicalname = physicalname_key;
				physicalnamecount = SDL_arraysize(physicalname_key);
				break;
			case 0xd76afdc0 /* "mouse_axis" */:
				inputtype = TYPE_MOUSE_AXIS;
				physicalname = physicalname_mouseaxis;
				physicalnamecount = SDL_arraysize(physicalname_mouseaxis);
				break;
			case 0xbe730575 /* "mouse_button" */:
				inputtype = TYPE_MOUSE_BUTTON;
				physicalname = physicalname_mousebutton;
				physicalnamecount = SDL_arraysize(physicalname_mousebutton);
				break;
			case 0x4b1fb051 /* "joystick_axis" */:
				inputtype = TYPE_JOYSTICK_AXIS;
				break;
			case 0xb084d264 /* "joystick_button" */:
				inputtype = TYPE_JOYSTICK_BUTTON;
				break;
			default:
				inputtype = NUM_TYPES;
				break;
			}

			// get properties
			int device = 0;
			element->QueryIntAttribute("device", &device);
			int control = 0;
			element->QueryIntAttribute("control", &control);
			if (physicalname)
			{
				const char *controlname = element->Attribute("control");
				for (int i = 0; i < physicalnamecount; ++i)
				{
					if (physicalname[i] != NULL && _stricmp(controlname, physicalname[i]) == 0)
					{
						control = i;
						break;
					}
				}
			}
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
void Input::Configure(const tinyxml2::XMLElement *element)
{
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureItem(child);
	}
}
