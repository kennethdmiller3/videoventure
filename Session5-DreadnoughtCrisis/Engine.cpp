#include "StdAfx.h"

#include "Entity.h"
#include "Link.h"
#include "Collidable.h"

namespace Database
{
	Typed<float> enginebasetemplate(0x8e6d1598 /* "enginebasetemplate" */);
	Typed<float> engineaddtemplate(0xbe47c2c0 /* "engineaddtemplate" */);
	Typed<float> setspeed(0x3631419e /* "setspeed" */);

	namespace Loader
	{

		static void EngineBaseConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			float &enginebase = Database::enginebasetemplate.Open(aId);
			element->QueryFloatAttribute("speed", &enginebase);
			Database::enginebasetemplate.Close(aId);
		}
		Configure enginebaseconfigure(0xeb9ca706 /* "enginebase" */, EngineBaseConfigure);

		static void EngineAddConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			float &engineadd = Database::engineaddtemplate.Open(aId);
			element->QueryFloatAttribute("speed", &engineadd);
			Database::engineaddtemplate.Close(aId);
		}
		Configure engineaddconfigure(0xc31e166e /* "engineadd" */, EngineAddConfigure);
	}

	namespace Initializer
	{
		static void ApplySpeed(unsigned int aId, float aSpeed)
		{
			if (Entity *entity = Database::entity.Get(aId))
			{
				Vector2 velocity(entity->GetTransform().Rotate(Vector2(0, aSpeed)));
				entity->SetVelocity(velocity);
				if (CollidableBody *body = Database::collidablebody.Get(aId))
				{
					Collidable::SetVelocity(body, velocity);
				}
			}
		}

		static void EngineBaseActivate(unsigned int aId)
		{
			float &setspeed = Database::setspeed.Open(aId);
			setspeed = Database::enginebasetemplate.Get(aId);
			ApplySpeed(aId, setspeed);
			Database::setspeed.Close(aId);
		}
		Activate enginebaseactivate(0x8e6d1598 /* "enginebasetemplate" */, EngineBaseActivate);

		static void EngineBaseDeactivate(unsigned int aId)
		{
		}
		Deactivate enginebasedeactivate(0x8e6d1598 /* "enginebasetemplate" */, EngineBaseDeactivate);

		static void EngineAddApply(unsigned int aId, float aDelta)
		{
			float &setspeed = Database::setspeed.Open(aId);
			setspeed += aDelta;
			ApplySpeed(aId, setspeed);
			Database::setspeed.Close(aId);
		}

		static void EngineAddActivate(unsigned int aId)
		{
			// backtrack to the entity containing the base speed
			for (unsigned int id = aId; id != 0; id = Database::backlink.Get(id))
			{
				if (Database::setspeed.Find(id))
				{
					EngineAddApply(id, Database::engineaddtemplate.Get(aId));
					break;
				}
			}
		}
		Activate engineaddactivate(0xbe47c2c0 /* "engineaddtemplate" */, EngineAddActivate);

		static void EngineAddDeactivate(unsigned int aId)
		{
			// backtrack to the entity containing the base speed
			for (unsigned int id = aId; id != 0; id = Database::backlink.Get(id))
			{
				if (Database::setspeed.Find(id))
				{
					EngineAddApply(id, -Database::engineaddtemplate.Get(aId));
					break;
				}
			}
		}
		Deactivate engineadddeactivate(0xbe47c2c0 /* "engineaddtemplate" */,EngineAddDeactivate);
	}
}
