#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsWheelJoint.h"

static bool ConfigureWheelJointItem(const tinyxml2::XMLElement *element, b2WheelJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchorA.x);
		element->QueryFloatAttribute("y", &joint.localAnchorA.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchorB.x);
		element->QueryFloatAttribute("y", &joint.localAnchorB.y);
		return true;

	case 0x6d2badf4 /* "axis" */:
		element->QueryFloatAttribute("x", &joint.localAxisA.x);
		element->QueryFloatAttribute("y", &joint.localAxisA.y);
		return true;

	case 0xcaf08472 /* "motor" */:
		element->QueryFloatAttribute("torque", &joint.maxMotorTorque);
		element->QueryFloatAttribute("speed", &joint.motorSpeed);
		joint.enableMotor = true;
		return true;

	case 0x4dc817ac /* "suspension" */:
		element->QueryFloatAttribute("frequency", &joint.frequencyHz);
		element->QueryFloatAttribute("damping", &joint.dampingRatio);
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2WheelJointDef> > wheeljointdef(0xd2e0447f /* "wheeljointdef" */);

	namespace Loader
	{
		static void WheelJointConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			Typed<b2WheelJointDef> defs = Database::wheeljointdef.Open(aId);

			// get the sub-identifier
			unsigned int aSubId;
			if (const char *name = element->Attribute("name"))
				aSubId = Hash(name);
			else
				aSubId = defs.GetCount() + 1;

			// configure the joint definition
			b2WheelJointDef &def = defs.Open(aSubId);
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				ConfigureWheelJointItem(child, def);
			}
			defs.Close(aSubId);

			Database::wheeljointdef.Close(aId);
		}
		Configure wheeljointconfigure(0xdafe5c18 /* "wheeljoint" */, WheelJointConfigure);
	}

	namespace Initializer
	{
		static void WheelJointPostActivate(unsigned int aId)
		{
			for (Database::Typed<b2WheelJointDef>::Iterator itor(&Database::wheeljointdef.Get(aId)); itor.IsValid(); ++itor)
			{
				b2WheelJointDef def(itor.GetValue());
				UnpackJointDef(def, aId);
				if (def.bodyA && def.bodyB)
				{
					Collidable::GetWorld()->CreateJoint(&def);
				}
			}
		}
		PostActivate wheeljointpostactivate(0xd2e0447f /* "wheeljointdef" */, WheelJointPostActivate);
	}
}
