#include "StdAfx.h"
#include "Input.h"

namespace Database
{
	Typed<Typed<Input::Binding> > inputbinding;
}

Input::Input(void)
{
	memset(value, 0, sizeof(value));
}

Input::~Input(void)
{
}

void Input::Bind(LOGICAL aLogical, int aType, int aDevice, int aControl, float aDeadzone, float aScale)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	Database::Typed<Input::Binding> &bindings = Database::inputbinding.Open(aPhysical);
	Binding &binding = bindings.Open(aLogical);
	binding.target = aLogical;
	binding.deadzone = aDeadzone;
	binding.scale = aScale;
	binding.previous = 0.0f;
	binding.pressed = false;
	binding.released = false;
	bindings.Close(aLogical);
	Database::inputbinding.Close(aPhysical);
}

void Input::Update(void)
{
	float scale;
	
	// limit magnitude of move control 1
	scale = value[MOVE_VERTICAL]*value[MOVE_VERTICAL]+value[MOVE_HORIZONTAL]*value[MOVE_HORIZONTAL];
	if (scale > 1.0f)
		scale = 1.0f / sqrtf(scale);
	else
		scale = 1.0f;
	output[MOVE_VERTICAL] = value[MOVE_VERTICAL] * scale;
	output[MOVE_HORIZONTAL] = value[MOVE_HORIZONTAL] * scale;

	// limit magnitude of aim control to 1
	scale = value[AIM_VERTICAL]*value[AIM_VERTICAL]+value[AIM_HORIZONTAL]*value[AIM_HORIZONTAL];
	if (scale > 1.0f)
		scale = 1.0f / sqrtf(scale);
	else
		scale = 1.0f;
	output[AIM_VERTICAL] = value[AIM_VERTICAL] * scale;
	output[AIM_HORIZONTAL] = value[AIM_HORIZONTAL] * scale;

	// limit magnitude of fire control to 1
	output[FIRE_PRIMARY] = std::min(std::max(value[FIRE_PRIMARY], -1.0f), 1.0f);
}

void Input::Step(void)
{
	for (Database::Typed<Database::Typed<Input::Binding> >::Iterator inputitor(&Database::inputbinding); inputitor.IsValid(); ++inputitor)
	{
		for (Database::Typed<Input::Binding>::Iterator bindingitor(&inputitor.GetValue()); bindingitor.IsValid(); ++bindingitor)
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

void Input::OnAxis(int aType, int aDevice, int aControl, float aValue)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	for (Database::Typed<Input::Binding>::Iterator bindingitor(Database::inputbinding.Find(aPhysical)); bindingitor.IsValid(); ++bindingitor)
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

void Input::OnPress(int aType, int aDevice, int aControl)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	for (Database::Typed<Input::Binding>::Iterator bindingitor(Database::inputbinding.Find(aPhysical)); bindingitor.IsValid(); ++bindingitor)
	{
		Binding &binding = const_cast<Binding &>(bindingitor.GetValue());
		if (!binding.pressed)
		{
			value[binding.target] += binding.scale;
			binding.pressed = true;
		}
	}
}

void Input::OnRelease(int aType, int aDevice, int aControl)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	for (Database::Typed<Input::Binding>::Iterator bindingitor(Database::inputbinding.Find(aPhysical)); bindingitor.IsValid(); ++bindingitor)
	{
		Binding &binding = const_cast<Binding &>(bindingitor.GetValue());
		binding.released = true;
	}
}
