#include "StdAfx.h"
#include "PhysicsUtilities.h"
#include "Collidable.h"

bool ConfigureJointItem(const tinyxml2::XMLElement *element, b2JointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0x115ce60c /* "body1" */:
		{
			const char *name = element->Attribute("name");
			joint.bodyA = reinterpret_cast<b2Body *>(name ? Hash(name) : 0);
		}
		return true;

	case 0x145ceac5 /* "body2" */:
		{
			const char *name = element->Attribute("name");
			joint.bodyB = reinterpret_cast<b2Body *>(name ? Hash(name) : 0);
		}
		return true;

	case 0x2c5d8028 /* "collideconnected" */:
		{
			element->QueryBoolAttribute("value", &joint.collideConnected);
		}
		return true;

	default:
		return false;
	}
}

void UnpackJointDef(b2JointDef &aDef, unsigned int aId)
{
	aDef.userData = NULL;
	unsigned int idA = reinterpret_cast<unsigned int>(aDef.bodyA);
	aDef.bodyA = Database::collidablebody.Get(idA ? idA : aId);
	unsigned int idB = reinterpret_cast<unsigned int>(aDef.bodyB);
	aDef.bodyB = Database::collidablebody.Get(idB ? idB : aId);
}