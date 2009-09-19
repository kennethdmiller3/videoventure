#pragma once

#include "Vector2.h"

class AlignedBox2
{
public:

	Vector2 min;
	Vector2 max;

public:

	AlignedBox2(void)
	{
	}

	AlignedBox2(const Vector2 &min, const Vector2 &max)
		: min(min), max(max)
	{
	}

	~AlignedBox2(void)
	{
	}
};
