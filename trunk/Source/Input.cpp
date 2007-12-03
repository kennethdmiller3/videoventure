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
	Binding &binding = map[aPhysical];
	binding.target = aLogical;
	binding.deadzone = aDeadzone;
	binding.scale = aScale;
	binding.previous = 0.0f;
	binding.pressed = false;
	binding.released = false;
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
	Map::iterator itor(map.find(aPhysical));
	if (itor != map.end())
	{
		Binding &binding = itor->second;
		if (aValue < -binding.deadzone)
			aValue = (aValue + binding.deadzone) * binding.scale;
		else if (aValue > binding.deadzone)
			aValue = (aValue - binding.deadzone) * binding.scale;
		else
			aValue = 0.0f;
		value[binding.target] += aValue - binding.previous;
		binding.previous = aValue;
	}
}

void Input::OnPress(int aType, int aDevice, int aControl)
{
	int aPhysical = (aType << 24) | (aDevice << 16) | aControl;
	Map::iterator itor(map.find(aPhysical));
	if (itor != map.end())
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
	Map::iterator itor(map.find(aPhysical));
	if (itor != map.end())
	{
		Binding &binding = itor->second;
		binding.released = true;
	}
}
