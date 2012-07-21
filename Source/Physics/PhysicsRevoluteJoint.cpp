#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsRevoluteJoint.h"

// Chipmunk includes
#pragma message( "chipmunk" )
#include "chipmunk/chipmunk.h"

static bool ConfigureRevoluteJointItem(const tinyxml2::XMLElement *element, RevoluteJointDef &joint, unsigned int id)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.mAnchorA.x);
		element->QueryFloatAttribute("y", &joint.mAnchorA.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.mAnchorB.x);
		element->QueryFloatAttribute("y", &joint.mAnchorB.y);
		return true;

	case 0xad544418 /* "angle" */:
		if (element->QueryFloatAttribute("value", &joint.mRefAngle) == tinyxml2::XML_SUCCESS)
			joint.mRefAngle *= float(M_PI)/180.0f;
		return true;

	case 0x32dad934 /* "limit" */:
		joint.mEnableLimit = true;
		if (element->QueryFloatAttribute("lower", &joint.mMinAngle) == tinyxml2::XML_SUCCESS)
			joint.mMinAngle *= float(M_PI) / 180.0f;
		if (element->QueryFloatAttribute("upper", &joint.mMaxAngle) == tinyxml2::XML_SUCCESS)
			joint.mMaxAngle *= float(M_PI) / 180.0f;
		return true;

	case 0xcaf08472 /* "motor" */:
		/*
		element->QueryFloatAttribute("torque", &joint.maxMotorTorque);
		if (element->QueryFloatAttribute("speed", &joint.motorSpeed) == tinyxml2::XML_SUCCESS)
			joint.motorSpeed *= float(M_PI) / 180.0f;
		joint.enableMotor = true;
		*/
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<RevoluteJointDef> > revolutejointdef(0x2af4a6c0 /* "revolutejointdef" */);

	namespace Loader
	{
		static void RevoluteJointConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			Typed<RevoluteJointDef> defs = Database::revolutejointdef.Open(aId);

			// get the sub-identifier
			unsigned int aSubId;
			if (const char *name = element->Attribute("name"))
				aSubId = Hash(name);
			else
				aSubId = defs.GetCount() + 1;

			// configure the joint definition
			RevoluteJointDef &def = defs.Open(aSubId);
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				ConfigureRevoluteJointItem(child, def, aId);
			}
			defs.Close(aSubId);

			Database::revolutejointdef.Close(aId);
		}
		Configure revolutejointconfigure(0xef2f9539 /* "revolutejoint" */, RevoluteJointConfigure);
	}

	namespace Initializer
	{
		static void RevoluteJointPostActivate(unsigned int aId)
		{
			for (Database::Typed<RevoluteJointDef>::Iterator itor(&Database::revolutejointdef.Get(aId)); itor.IsValid(); ++itor)
			{
				const RevoluteJointDef &def = itor.GetValue();

				// world
				cpSpace *world = Collidable::GetWorld();

				// collidable bodies
				unsigned int idA = def.mIdA ? def.mIdA : aId;
				cpBody *a = Database::collidablebody.Get(idA);
				if (a == NULL)
				{
					DebugPrint("revolute joint A \"%s\" (%08x) has no body\n", Database::name.Get(idA).c_str(), idA);
					continue;
				}
				unsigned int idB = def.mIdB ? def.mIdB : aId;
				cpBody *b = Database::collidablebody.Get(idB);
				if (b == NULL)
				{
					DebugPrint("revolute joint B \"%s\" (%08x) has no body\n", Database::name.Get(idB).c_str(), idB);
					continue;
				}

				// create pivot constraint
				cpSpaceAddConstraint(world,
					cpPivotJointNew2(a, b, cpv(def.mAnchorA.x, def.mAnchorA.y), cpv(def.mAnchorB.x, def.mAnchorB.y)));

				if (def.mEnableLimit)
				{
					// create rotary constraint
					cpSpaceAddConstraint(world,
						cpRotaryLimitJointNew(a, b, def.mRefAngle + def.mMinAngle, def.mRefAngle + def.mMaxAngle));
				}
			}
		}
		PostActivate revolutejointpostactivate(0x2af4a6c0 /* "revolutejointdef" */, RevoluteJointPostActivate);
	}
}
