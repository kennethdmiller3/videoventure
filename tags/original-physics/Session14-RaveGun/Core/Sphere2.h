#pragma once

#include "Vector2.h"

class Sphere2
{
public:
	Vector2 p;
	float r;

public:
	Sphere2(void)
	{
	}

	Sphere2(const Vector2 &p, float r)
		: p(p), r(r)
	{
	}

	~Sphere2(void)
	{
	}
};
