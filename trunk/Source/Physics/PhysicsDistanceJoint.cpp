#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsDistanceJoint.h"

static bool ConfigureDistanceJointItem(const TiXmlElement *element, b2DistanceJointDef &joint)
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

	default:
		return ConfigureJointItem(element, joint);
	}
}

namespace Database
{
	Typed<Typed<b2DistanceJointDef> > distancejointdef(0x3caa9665 /* "distancejointdef" */);

	namespace Loader
	{
		class DistanceJointLoader
		{
		public:
			DistanceJointLoader()
			{
				AddConfigure(0x6932d1ee /* "distancejoint" */, Entry(this, &DistanceJointLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Typed<b2DistanceJointDef> defs = Database::distancejointdef.Open(aId);

				// get the sub-identifier
				unsigned int aSubId;
				if (const char *name = element->Attribute("name"))
					aSubId = Hash(name);
				else
					aSubId = defs.GetCount() + 1;

				// configure the joint definition
				b2DistanceJointDef &def = defs.Open(aSubId);
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					ConfigureDistanceJointItem(child, def);
				}
				defs.Close(aSubId);

				Database::distancejointdef.Close(aId);
			}
		}
		distancejointloader;
	}

	namespace Initializer
	{
		class DistanceJointInitializer
		{
		public:
			DistanceJointInitializer()
			{
				AddPostActivate(0x3caa9665 /* "distancejointdef" */, Entry(this, &DistanceJointInitializer::PostActivate));
			}

			void PostActivate(unsigned int aId)
			{
				for (Database::Typed<b2DistanceJointDef>::Iterator itor(&Database::distancejointdef.Get(aId)); itor.IsValid(); ++itor)
				{
					b2DistanceJointDef def(itor.GetValue());
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
		distancejointinitializer;
	}
}
