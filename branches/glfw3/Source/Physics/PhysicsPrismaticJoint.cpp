#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsPrismaticJoint.h"

static bool ConfigurePrismaticJointItem(const tinyxml2::XMLElement *element, b2PrismaticJointDef &joint)
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

	case 0xad544418 /* "angle" */:
		if (element->QueryFloatAttribute("value", &joint.referenceAngle) == tinyxml2::XML_SUCCESS)
			joint.referenceAngle *= float(M_PI)/180.0f;
		return true;

	case 0x32dad934 /* "limit" */:
		element->QueryFloatAttribute("lower", &joint.lowerTranslation);
		element->QueryFloatAttribute("upper", &joint.upperTranslation);
		joint.enableLimit = true;
		return true;

	case 0xcaf08472 /* "motor" */:
		element->QueryFloatAttribute("force", &joint.maxMotorForce);
		element->QueryFloatAttribute("speed", &joint.motorSpeed);
		joint.enableMotor = true;
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2PrismaticJointDef> > prismaticjointdef(0x85eb7374 /* "prismaticjointdef" */);

	namespace Loader
	{
		static void PrismaticJointConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			Typed<b2PrismaticJointDef> defs = Database::prismaticjointdef.Open(aId);

			// get the sub-identifier
			unsigned int aSubId;
			if (const char *name = element->Attribute("name"))
				aSubId = Hash(name);
			else
				aSubId = defs.GetCount() + 1;

			// configure the joint definition
			b2PrismaticJointDef &def = defs.Open(aSubId);
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				ConfigurePrismaticJointItem(child, def);
			}
			defs.Close(aSubId);

			Database::prismaticjointdef.Close(aId);
		}
		Configure prismaticjointconfigure(0x4954853d /* "prismaticjoint" */, PrismaticJointConfigure);
	}

	namespace Initializer
	{
		static void PrismaticJointPostActivate(unsigned int aId)
		{
			for (Database::Typed<b2PrismaticJointDef>::Iterator itor(&Database::prismaticjointdef.Get(aId)); itor.IsValid(); ++itor)
			{
				b2PrismaticJointDef def(itor.GetValue());
				UnpackJointDef(def, aId);
				if (def.bodyA && def.bodyB)
				{
					Collidable::GetWorld()->CreateJoint(&def);
				}
			}
		}
		PostActivate prismaticjointpostactivate(0x85eb7374 /* "prismaticjointdef" */, PrismaticJointPostActivate);
	}
}
