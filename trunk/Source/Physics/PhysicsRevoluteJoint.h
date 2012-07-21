#pragma once

#include "PhysicsUtilities.h"

class RevoluteJointDef : public ConstraintDef
{
public:
	RevoluteJointDef()
		: ConstraintDef()
		, mAnchorA(0.0f, 0.0f)
		, mAnchorB(0.0f, 0.0f)
		, mEnableLimit(false)
		, mRefAngle(0.0f)
		, mMinAngle(-FLT_MAX)
		, mMaxAngle(FLT_MAX)
	{
	}

public:
	Vector2 mAnchorA;
	Vector2 mAnchorB;
	bool mEnableLimit;
	float mRefAngle;
	float mMinAngle;
	float mMaxAngle;
};

namespace Database
{
	extern Typed<Typed<RevoluteJointDef> > revolutejointdef;
}