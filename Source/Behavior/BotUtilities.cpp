#include "StdAfx.h"

#include "BotUtilities.h"
#include "Entity.h"
#include "Ship.h"

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
	float t;
	if (d <= 0.0f)
	{
		// no real roots; get time of closest approach
		t = -b / a;
	}
	else
	{
		float sqrtd = sqrtf(d);
		float f = b > 0 ? -(b + sqrtd) : -(b - sqrtd);

		// if root 1 is not positive...
		if (f * a <= 0.0f)
		{
			// use root 2
			t = c / f;
		}

		// else if root 2 is not positive...
		else if (c * f <= 0.0f)
		{
			// use root 1
			t = f / a;
		}
		else
		{
			// get both roots
			float t1 = f / a;
			float t2 = c / f;

			// use the minimum of the two
			t = std::min(t1, t2);
		}
	}

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

float SteerTowards(const Entity *aEntity, const Vector2 &aLocalDir)
{
	// get physics properties
	// TO DO: replace this with an control property
	const ShipTemplate &ship = Database::shiptemplate.Get(aEntity->GetId());	// <-- hack!
	if (ship.mMaxOmega != 0.0f)
	{
		// angle to target
		float aimAngle = -atan2f(aLocalDir.x, aLocalDir.y);

		// turn towards target direction
		return Clamp(aimAngle / (ship.mMaxOmega * sim_step), -1.0f, 1.0f);
	}

	return 0.0f;
}
