#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsRopeJoint.h"

static bool ConfigureRopeJointItem(const tinyxml2::XMLElement *element, b2RopeJointDef &joint)
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

	case 0x5d6614a1 /* "maxlength" */:
		element->QueryFloatAttribute("value", &joint.maxLength);
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2RopeJointDef> > ropejointdef(0xadbc04ee /* "ropejointdef" */);

	namespace Loader
	{
		static void RopeJointConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			Typed<b2RopeJointDef> defs = Database::ropejointdef.Open(aId);

			// get the sub-identifier
			unsigned int aSubId;
			if (const char *name = element->Attribute("name"))
				aSubId = Hash(name);
			else
				aSubId = defs.GetCount() + 1;

			// configure the joint definition
			b2RopeJointDef &def = defs.Open(aSubId);
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				ConfigureRopeJointItem(child, def);
			}
			defs.Close(aSubId);

			Database::ropejointdef.Close(aId);
		}
		Configure ropejointconfigure(0x84e3150f /* "ropejoint" */, RopeJointConfigure);
	}

	namespace Initializer
	{
		static void RopeJointPostActivate(unsigned int aId)
		{
			for (Database::Typed<b2RopeJointDef>::Iterator itor(&Database::ropejointdef.Get(aId)); itor.IsValid(); ++itor)
			{
				b2RopeJointDef def(itor.GetValue());
				UnpackJointDef(def, aId);
				if (def.bodyA && def.bodyB)
				{
					Collidable::GetWorld()->CreateJoint(&def);
				}
			}
		}
		PostActivate ropejointpostactivate(0xadbc04ee /* "ropejointdef" */, RopeJointPostActivate);
	}
}
