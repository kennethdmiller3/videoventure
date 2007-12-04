#include "StdAfx.h"
#include "Input.h"

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
	Binding binding;
	binding.target = aLogical;
	binding.deadzone = aDeadzone;
	binding.scale = aScale;
	binding.previous = 0.0f;
	binding.pressed = false;
	binding.released = false;
	map.insert(Map::value_type(aPhysical, binding));
}

void Input::Update(void)
{
	for (Map::iterator itor = map.begin(); itor != map.end(); itor++)
	{
		Binding &binding = itor->second;
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

void Input::OnAxis(int aType, int aDevice, int aControl, float aValue)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	std::pair<Map::iterator, Map::iterator> p = map.equal_range(aPhysical);
	for (Map::iterator itor = p.first; itor != p.second; ++itor)
	{
		Binding &binding = itor->second;
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
	std::pair<Map::iterator, Map::iterator> p = map.equal_range(aPhysical);
	for (Map::iterator itor = p.first; itor != p.second; ++itor)
	{
		Binding &binding = itor->second;
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
	std::pair<Map::iterator, Map::iterator> p = map.equal_range(aPhysical);
	for (Map::iterator itor = p.first; itor != p.second; ++itor)
	{
		Binding &binding = itor->second;
		binding.released = true;
	}
}
