#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsWeldJoint.h"

static bool ConfigureWeldJointItem(const tinyxml2::XMLElement *element, b2WeldJointDef &joint)
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
		if (element->QueryFloatAttribute("value", &joint.referenceAngle) == tinyxml2::XML_SUCCESS)
			joint.referenceAngle *= float(M_PI)/180.0f;
		return true;

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2WeldJointDef> > weldjointdef(0xf4218892 /* "weldjointdef" */);

	namespace Loader
	{
		class WeldJointLoader
		{
		public:
			WeldJointLoader()
			{
				AddConfigure(0x1190c943 /* "weldjoint" */, Entry(this, &WeldJointLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				Typed<b2WeldJointDef> defs = Database::weldjointdef.Open(aId);

				// get the sub-identifier
				unsigned int aSubId;
				if (const char *name = element->Attribute("name"))
					aSubId = Hash(name);
				else
					aSubId = defs.GetCount() + 1;

				// configure the joint definition
				b2WeldJointDef &def = defs.Open(aSubId);
				for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					ConfigureWeldJointItem(child, def);
				}
				defs.Close(aSubId);

				Database::weldjointdef.Close(aId);
			}
		}
		weldjointloader;
	}

	namespace Initializer
	{
		class WeldJointInitializer
		{
		public:
			WeldJointInitializer()
			{
				AddPostActivate(0xf4218892 /* "weldjointdef" */, Entry(this, &WeldJointInitializer::PostActivate));
			}

			void PostActivate(unsigned int aId)
			{
				for (Database::Typed<b2WeldJointDef>::Iterator itor(&Database::weldjointdef.Get(aId)); itor.IsValid(); ++itor)
				{
					b2WeldJointDef def(itor.GetValue());
					UnpackJointDef(def, aId);
					if (def.bodyA && def.bodyB)
					{
						Collidable::GetWorld()->CreateJoint(&def);
					}
				}
			}
		}
		weldjointinitializer;
	}
}
