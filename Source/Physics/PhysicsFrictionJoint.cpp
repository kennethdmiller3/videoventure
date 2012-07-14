#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsFrictionJoint.h"

static bool ConfigureFrictionJointItem(const tinyxml2::XMLElement *element, b2FrictionJointDef &joint)
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

	case 0xe851a9b8 /* "maxforce" */:
		element->QueryFloatAttribute("value", &joint.maxForce);
		return true;

	case 0xab903537 /* "maxtorque" */:
		element->QueryFloatAttribute("value", &joint.maxTorque);
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2FrictionJointDef> > frictionjointdef(0x9d9badd4 /* "frictionjointdef" */);

	namespace Loader
	{
		static void FrictionJointConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			Typed<b2FrictionJointDef> defs = Database::frictionjointdef.Open(aId);

			// get the sub-identifier
			unsigned int aSubId;
			if (const char *name = element->Attribute("name"))
				aSubId = Hash(name);
			else
				aSubId = defs.GetCount() + 1;

			// configure the joint definition
			b2FrictionJointDef &def = defs.Open(aSubId);
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				ConfigureFrictionJointItem(child, def);
			}
			defs.Close(aSubId);

			Database::frictionjointdef.Close(aId);
		}
		Configure frictionjointconfigure(0x6c34561d /* "frictionjoint" */, FrictionJointConfigure);
	}

	namespace Initializer
	{
		static void FrictionJointPostActivate(unsigned int aId)
		{
			for (Database::Typed<b2FrictionJointDef>::Iterator itor(&Database::frictionjointdef.Get(aId)); itor.IsValid(); ++itor)
			{
				b2FrictionJointDef def(itor.GetValue());
				UnpackJointDef(def, aId);
				if (def.bodyA && def.bodyB)
				{
					Collidable::GetWorld()->CreateJoint(&def);
				}
			}
		}
		PostActivate frictionjointpostactivate(0x9d9badd4 /* "frictionjointdef" */, FrictionJointPostActivate);
	}
}
