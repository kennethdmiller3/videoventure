#include "StdAfx.h"
#include "PhysicsUtilities.h"

bool ConfigureJointItem(const TiXmlElement *element, b2JointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x115ce60c /* "body1" */:
		{
			const char *name = element->Attribute("name");
			joint.body1 = reinterpret_cast<b2Body *>(name ? Hash(name) : 0);
		}
		return true;

	case 0x145ceac5 /* "body2" */:
		{
			const char *name = element->Attribute("name");
			joint.body2 = reinterpret_cast<b2Body *>(name ? Hash(name) : 0);
		}
		return true;

	case 0x2c5d8028 /* "collideconnected" */:
		{
			int collide = joint.collideConnected;
			element->QueryIntAttribute("value", &collide);
			joint.collideConnected = collide != 0;
		}
		return true;

	default:
		return false;
	}
}
