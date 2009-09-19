#include "StdAfx.h"

#include "BotUtilities.h"
#include "..\Entity.h"

Vector2 Intercept(float aLeading, const Vector2 &aPosition, const Vector2 &aVelocity)
{
#if 1
	// compute quadratic formula coefficients
	float a = aVelocity.Dot(aVelocity) - aLeading * aLeading;
	float b = aPosition.Dot(aVelocity);		// divided by 2
	float c = aPosition.Dot(aPosition);

	// compute the discriminant
	float d = b * b - a * c;

	// compute the time to intersection
	if (d > 0.0f)
		b += sqrtf(d);
	float t;
	if (fabsf(a) > FLT_EPSILON)
		t = -b / a;
	else if (fabsf(b) > FLT_EPSILON)
		t = c / -b;
	else
		t = 0.0f;

	// prevent negative time
	if (t < 0.0f)
		t = 0.0f;

	// return intersection position
	return aPosition + t * aVelocity;
#else
	// extremely simple leading based on distance
	return aPosition + aVelocity * aPosition.Length() / aLeading;
#endif
}

Vector2 TargetDir(float aLeading, const Entity *aEntity, const Entity *aTargetEntity, const Vector2 &aOffset)
{
	// direction to target
	Vector2 targetDir(aTargetEntity->GetTransform().Transform(aOffset) - aEntity->GetPosition());

	// get target lead position
	if (aLeading != 0.0f)
	{
		targetDir = Intercept(aLeading, targetDir, aTargetEntity->GetVelocity() - aEntity->GetVelocity());
	}

	// return direction
	return targetDir;
}
