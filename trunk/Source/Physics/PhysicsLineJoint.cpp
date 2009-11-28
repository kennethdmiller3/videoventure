#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsLineJoint.h"

static bool ConfigureLineJointItem(const TiXmlElement *element, b2LineJointDef &joint)
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
	Typed<Typed<b2LineJointDef> > linejointdef(0xce9fc310 /* "linejointdef" */);

	namespace Loader
	{
		class LineJointLoader
		{
		public:
			LineJointLoader()
			{
				AddConfigure(0xa59c5ee9 /* "linejoint" */, Entry(this, &LineJointLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Typed<b2LineJointDef> defs = Database::linejointdef.Open(aId);

				// get the sub-identifier
				unsigned int aSubId;
				if (const char *name = element->Attribute("name"))
					aSubId = Hash(name);
				else
					aSubId = defs.GetCount() + 1;

				// configure the joint definition
				b2LineJointDef &def = defs.Open(aSubId);
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					ConfigureLineJointItem(child, def);
				}
				defs.Close(aSubId);

				Database::linejointdef.Close(aId);
			}
		}
		linejointloader;
	}

	namespace Initializer
	{
		class LineJointInitializer
		{
		public:
			LineJointInitializer()
			{
				AddPostActivate(0xce9fc310 /* "linejointdef" */, Entry(this, &LineJointInitializer::PostActivate));
			}

			void PostActivate(unsigned int aId)
			{
				for (Database::Typed<b2LineJointDef>::Iterator itor(&Database::linejointdef.Get(aId)); itor.IsValid(); ++itor)
				{
					b2LineJointDef def(itor.GetValue());
					UnpackJointDef(def, aId);
					if (def.bodyA && def.bodyB)
					{
						Collidable::GetWorld()->CreateJoint(&def);
					}
				}
			}
		}
		linejointinitializer;
	}
}
