#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsRevoluteJoint.h"

static bool ConfigureRevoluteJointItem(const TiXmlElement *element, b2RevoluteJointDef &joint)
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

	case 0xad544418 /* "angle" */:
		if (element->QueryFloatAttribute("value", &joint.referenceAngle) == TIXML_SUCCESS)
			joint.referenceAngle *= float(M_PI)/180.0f;
		return true;

	case 0x32dad934 /* "limit" */:
		if (element->QueryFloatAttribute("lower", &joint.lowerAngle) == TIXML_SUCCESS)
			joint.lowerAngle *= float(M_PI) / 180.0f;
		if (element->QueryFloatAttribute("upper", &joint.upperAngle) == TIXML_SUCCESS)
			joint.upperAngle *= float(M_PI) / 180.0f;
		joint.enableLimit = true;
		return true;

	case 0xcaf08472 /* "motor" */:
		element->QueryFloatAttribute("torque", &joint.maxMotorTorque);
		if (element->QueryFloatAttribute("speed", &joint.motorSpeed) == TIXML_SUCCESS)
			joint.motorSpeed *= float(M_PI) / 180.0f;
		joint.enableMotor = true;
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2RevoluteJointDef> > revolutejointdef(0x2af4a6c0 /* "revolutejointdef" */);

	namespace Loader
	{
		class RevoluteJointLoader
		{
		public:
			RevoluteJointLoader()
			{
				AddConfigure(0xef2f9539 /* "revolutejoint" */, Entry(this, &RevoluteJointLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Typed<b2RevoluteJointDef> defs = Database::revolutejointdef.Open(aId);

				// get the sub-identifier
				unsigned int aSubId;
				if (const char *name = element->Attribute("name"))
					aSubId = Hash(name);
				else
					aSubId = defs.GetCount() + 1;

				// configure the joint definition
				b2RevoluteJointDef &def = defs.Open(aSubId);
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					ConfigureRevoluteJointItem(child, def);
				}
				defs.Close(aSubId);

				Database::revolutejointdef.Close(aId);
			}
		}
		revolutejointloader;
	}

	namespace Initializer
	{
		class RevoluteJointInitializer
		{
		public:
			RevoluteJointInitializer()
			{
				AddPostActivate(0x2af4a6c0 /* "revolutejointdef" */, Entry(this, &RevoluteJointInitializer::PostActivate));
			}

			void PostActivate(unsigned int aId)
			{
				for (Database::Typed<b2RevoluteJointDef>::Iterator itor(&Database::revolutejointdef.Get(aId)); itor.IsValid(); ++itor)
				{
					b2RevoluteJointDef def(itor.GetValue());
					UnpackJointDef(def, aId);
					if (def.bodyA && def.bodyB)
					{
						Collidable::GetWorld()->CreateJoint(&def);
					}
				}
			}
		}
		revolutejointinitializer;
	}
}
