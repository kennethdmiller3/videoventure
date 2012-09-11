#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsPulleyJoint.h"

static bool ConfigurePulleyJointItem(const tinyxml2::XMLElement *element, b2PulleyJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe1acc15d /* "ground1" */:
		element->QueryFloatAttribute("x", &joint.groundAnchorA.x);
		element->QueryFloatAttribute("y", &joint.groundAnchorA.y);
		return true;

	case 0xdeacbca4 /* "ground2" */:
		element->QueryFloatAttribute("x", &joint.groundAnchorB.x);
		element->QueryFloatAttribute("y", &joint.groundAnchorB.y);
		return true;

	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchorA.x);
		element->QueryFloatAttribute("y", &joint.localAnchorA.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchorB.x);
		element->QueryFloatAttribute("y", &joint.localAnchorB.y);
		return true;

	case 0xa4c53aac /* "length1" */:
		element->QueryFloatAttribute("max", &joint.lengthA);
		return true;

	case 0xa7c53f65 /* "length2" */:
		element->QueryFloatAttribute("max", &joint.lengthB);
		return true;

	case 0xc1121e84 /* "ratio" */:
		element->QueryFloatAttribute("value", &joint.ratio);
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2PulleyJointDef> > pulleyjointdef(0x5f072ebb /* "pulleyjointdef" */);

	namespace Loader
	{
		static void PulleyJointConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			Typed<b2PulleyJointDef> defs = Database::pulleyjointdef.Open(aId);

			// get the sub-identifier
			unsigned int aSubId;
			if (const char *name = element->Attribute("name"))
				aSubId = Hash(name);
			else
				aSubId = defs.GetCount() + 1;

			// configure the joint definition
			b2PulleyJointDef &def = defs.Open(aSubId);
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				ConfigurePulleyJointItem(child, def);
			}
			defs.Close(aSubId);

			Database::pulleyjointdef.Close(aId);
		}
		Configure pulleyjointconfigure(0xdd003dc4 /* "pulleyjoint" */, PulleyJointConfigure);
	}

	namespace Initializer
	{
		static void PulleyJointPostActivate(unsigned int aId)
		{
			for (Database::Typed<b2PulleyJointDef>::Iterator itor(&Database::pulleyjointdef.Get(aId)); itor.IsValid(); ++itor)
			{
				b2PulleyJointDef def(itor.GetValue());
				UnpackJointDef(def, aId);
				if (def.bodyA && def.bodyB)
				{
					Collidable::GetWorld()->CreateJoint(&def);
				}
			}
		}
		PostActivate pulleyjointpostactivate(0x5f072ebb /* "pulleyjointdef" */, PulleyJointPostActivate);
	}
}
