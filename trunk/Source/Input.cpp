#include "StdAfx.h"
#include "Input.h"

Input::Input(void)
{
	memset(set, 0, sizeof(set));
	memset(clear, 0, sizeof(clear));
}

Input::~Input(void)
{
}

void Input::Start(void)
{
	// set up inputs
	for (int i = 0; i < NUM_LOGICAL; i++)
	{
		if (clear[i])
			set[i] = false;
		clear[i] = false;
	}
}

void Input::OnKeyDown(int aKey)
{
	Map::iterator itor(map.find(aKey));
	if (itor != map.end())
	{
		set[itor->second] = true;
	}
}

void Input::OnKeyUp(int aKey)
{
	Map::iterator itor(map.find(aKey));
	if (itor != map.end())
	{
		clear[itor->second] = true;
	}
}
