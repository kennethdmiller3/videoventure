#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsPrismaticJoint.h"

static bool ConfigurePrismaticJointItem(const TiXmlElement *element, b2PrismaticJointDef &joint)
{
	const char *name = element->Value();
	switch (Hash(name))
	{
	case 0xe155cf5f /* "anchor1" */:
		element->QueryFloatAttribute("x", &joint.localAnchor1.x);
		element->QueryFloatAttribute("y", &joint.localAnchor1.y);
		return true;

	case 0xe255d0f2 /* "anchor2" */:
		element->QueryFloatAttribute("x", &joint.localAnchor2.x);
		element->QueryFloatAttribute("y", &joint.localAnchor2.y);
		return true;

	case 0x6d2badf4 /* "axis" */:
		element->QueryFloatAttribute("x", &joint.localAxis1.x);
		element->QueryFloatAttribute("y", &joint.localAxis1.y);
		return true;

	case 0xad544418 /* "angle" */:
		if (element->QueryFloatAttribute("value", &joint.referenceAngle) == TIXML_SUCCESS)
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
		class PrismaticJointLoader
		{
		public:
			PrismaticJointLoader()
			{
				AddConfigure(0x4954853d /* "prismaticjoint" */, Entry(this, &PrismaticJointLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
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
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					ConfigurePrismaticJointItem(child, def);
				}
				defs.Close(aSubId);

				Database::prismaticjointdef.Close(aId);
			}
		}
		prismaticjointloader;
	}

	namespace Initializer
	{
		class PrismaticJointInitializer
		{
		public:
			PrismaticJointInitializer()
			{
				AddPostActivate(0x85eb7374 /* "prismaticjointdef" */, Entry(this, &PrismaticJointInitializer::PostActivate));
			}

			void PostActivate(unsigned int aId)
			{
				for (Database::Typed<b2PrismaticJointDef>::Iterator itor(&Database::prismaticjointdef.Get(aId)); itor.IsValid(); ++itor)
				{
					b2PrismaticJointDef def(itor.GetValue());
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
		prismaticjointinitializer;
	}
}
