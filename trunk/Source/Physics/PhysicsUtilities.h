#pragma once

class ConstraintDef
{
public:
	ConstraintDef()
		: mIdA(0)
		, mIdB(0)
		, mMaxForce(FLT_MAX)
	{
	}

public:
	unsigned int mIdA;
	unsigned int mIdB;
	float mMaxForce;
};

extern bool ConfigureJointItem(const tinyxml2::XMLElement *element, ConstraintDef &joint);
