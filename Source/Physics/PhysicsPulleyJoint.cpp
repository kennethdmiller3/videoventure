#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsPulleyJoint.h"

bool ConfigurePulleyJointItem(const TiXmlElement *element, b2PulleyJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe1acc15d /* "ground1" */:
		element->QueryFloatAttribute("x", &joint.groundAnchor1.x);
		element->QueryFloatAttribute("y", &joint.groundAnchor1.y);
		return true;

	case 0xdeacbca4 /* "ground2" */:
		element->QueryFloatAttribute("x", &joint.groundAnchor2.x);
		element->QueryFloatAttribute("y", &joint.groundAnchor2.y);
		return true;

	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchor1.x);
		element->QueryFloatAttribute("y", &joint.localAnchor1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchor2.x);
		element->QueryFloatAttribute("y", &joint.localAnchor2.y);
		return true;

	case 0xa4c53aac /* "length1" */:
		element->QueryFloatAttribute("max", &joint.maxLength1);
		return true;

	case 0xa7c53f65 /* "length2" */:
		element->QueryFloatAttribute("max", &joint.maxLength2);
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
		class PulleyJointLoader
		{
		public:
			PulleyJointLoader()
			{
				AddConfigure(0xdd003dc4 /* "pulleyjoint" */, Entry(this, &PulleyJointLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
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
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					ConfigurePulleyJointItem(child, def);
				}
				defs.Close(aSubId);

				Database::pulleyjointdef.Close(aId);
			}
		}
		pulleyjointloader;
	}

	namespace Initializer
	{
		class PulleyJointInitializer
		{
		public:
			PulleyJointInitializer()
			{
				AddPostActivate(0x5f072ebb /* "pulleyjointdef" */, Entry(this, &PulleyJointInitializer::PostActivate));
			}

			void PostActivate(unsigned int aId)
			{
				for (Database::Typed<b2PulleyJointDef>::Iterator itor(&Database::pulleyjointdef.Get(aId)); itor.IsValid(); ++itor)
				{
					b2PulleyJointDef def(itor.GetValue());
					def.userData = NULL;
					unsigned int id1 = reinterpret_cast<unsigned int>(def.body1);
					def.body1 = Database::collidablebody.Get(id1 ? id1 : aId);
					unsigned int id2 = reinterpret_cast<unsigned int>(def.body2);
					def.body2 = Database::collidablebody.Get(id2 ? id2 : aId);
					if (def.body1 && def.body2)
					{
						Collidable::GetWorld()->CreateJoint(&def);
					}
				}
			}
		}
		pulleyjointinitializer;
	}
}
