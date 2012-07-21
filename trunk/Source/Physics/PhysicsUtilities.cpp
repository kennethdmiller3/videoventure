#include "StdAfx.h"
#include "PhysicsUtilities.h"
#include "Collidable.h"

bool ConfigureJointItem(const tinyxml2::XMLElement *element, ConstraintDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x115ce60c /* "body1" */:
		{
			const char *name = element->Attribute("name");
			joint.mIdA = name ? Hash(name) : 0;
		}
		return true;

	case 0x145ceac5 /* "body2" */:
		{
			const char *name = element->Attribute("name");
			joint.mIdB = name ? Hash(name) : 0;
		}
		return true;

	default:
		return false;
	}
}
