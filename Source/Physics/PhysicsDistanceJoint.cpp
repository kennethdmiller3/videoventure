#include "StdAfx.h"
#include "Collidable.h"
#include "PhysicsUtilities.h"
#include "PhysicsDistanceJoint.h"


	/// The natural length between the anchor points.
	float32 length;

	/// The mass-spring-damper frequency in Hertz.
	float32 frequencyHz;

	/// The damping ratio. 0 = no damping, 1 = critical damping.
	float32 dampingRatio;

static bool ConfigureDistanceJointItem(const tinyxml2::XMLElement *element, b2DistanceJointDef &joint)
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

	case 0x83d03615 /* "length" */:
		element->QueryFloatAttribute("value", &joint.length);
		return true;

	case 0x2fb31c01 /* "frequency" */:
		element->QueryFloatAttribute("value", &joint.frequencyHz);
		return true;

	case 0xfb1994b4 /* "dampingratio" */:
		element->QueryFloatAttribute("value", &joint.dampingRatio);
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

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
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
				for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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
					UnpackJointDef(def, aId);
					if (def.bodyA && def.bodyB)
					{
						Collidable::GetWorld()->CreateJoint(&def);
					}
				}
			}
		}
		distancejointinitializer;
	}
}
