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
		class EngineBaseLoader
		{
		public:
			EngineBaseLoader()
			{
				AddConfigure(0xeb9ca706 /* "enginebase" */, Entry(this, &EngineBaseLoader::Configure));
			}

			~EngineBaseLoader()
			{
				RemoveConfigure(0xeb9ca706 /* "enginebase" */, Entry(this, &EngineBaseLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				float &enginebase = Database::enginebasetemplate.Open(aId);
				element->QueryFloatAttribute("speed", &enginebase);
				Database::enginebasetemplate.Close(aId);
			}
		}
		enginebaseloader;

		class EngineAddLoader
		{
		public:
			EngineAddLoader()
			{
				AddConfigure(0xc31e166e /* "engineadd" */, Entry(this, &EngineAddLoader::Configure));
			}

			~EngineAddLoader()
			{
				RemoveConfigure(0xc31e166e /* "engineadd" */, Entry(this, &EngineAddLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				float &engineadd = Database::engineaddtemplate.Open(aId);
				element->QueryFloatAttribute("speed", &engineadd);
				Database::engineaddtemplate.Close(aId);
			}
		}
		engineaddloader;
	}

	namespace Initializer
	{
		static void ApplySpeed(unsigned int aId, float aSpeed)
		{
			if (Entity *entity = Database::entity.Get(aId))
			{
				Vector2 velocity(entity->GetTransform().Rotate(Vector2(0, aSpeed)));
				entity->SetVelocity(velocity);
				if (b2Body *body = Database::collidablebody.Get(aId))
					body->SetLinearVelocity(velocity);
			}
		}

		class EngineBaseInitializer
		{
		public:
			EngineBaseInitializer()
			{
				AddActivate(0x8e6d1598 /* "enginebasetemplate" */, Entry(this, &EngineBaseInitializer::Activate));
				AddDeactivate(0x8e6d1598 /* "enginebasetemplate" */, Entry(this, &EngineBaseInitializer::Deactivate));
			}

			~EngineBaseInitializer()
			{
				RemoveActivate(0x8e6d1598 /* "enginebasetemplate" */, Entry(this, &EngineBaseInitializer::Activate));
				RemoveDeactivate(0x8e6d1598 /* "enginebasetemplate" */, Entry(this, &EngineBaseInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				float &setspeed = Database::setspeed.Open(aId);
				setspeed = Database::enginebasetemplate.Get(aId);
				ApplySpeed(aId, setspeed);
				Database::setspeed.Close(aId);
			}

			void Deactivate(unsigned int aId)
			{
			}
		}
		enginebaseinitializer;

		class EngineAddInitializer
		{
		public:
			EngineAddInitializer()
			{
				AddActivate(0xbe47c2c0 /* "engineaddtemplate" */, Entry(this, &EngineAddInitializer::Activate));
				AddDeactivate(0xbe47c2c0 /* "engineaddtemplate" */, Entry(this, &EngineAddInitializer::Deactivate));
			}

			~EngineAddInitializer()
			{
				RemoveActivate(0xbe47c2c0 /* "engineaddtemplate" */, Entry(this, &EngineAddInitializer::Activate));
				RemoveDeactivate(0xbe47c2c0 /* "engineaddtemplate" */, Entry(this, &EngineAddInitializer::Deactivate));
			}

			void Apply(unsigned int aId, float aDelta)
			{
				float &setspeed = Database::setspeed.Open(aId);
				setspeed += aDelta;
				ApplySpeed(aId, setspeed);
				Database::setspeed.Close(aId);
			}

			void Activate(unsigned int aId)
			{
				// backtrack to the entity containing the base speed
				for (unsigned int id = aId; id != 0; id = Database::backlink.Get(id))
				{
					if (Database::setspeed.Find(id))
					{
						Apply(id, Database::engineaddtemplate.Get(aId));
						break;
					}
				}
			}

			void Deactivate(unsigned int aId)
			{
				// backtrack to the entity containing the base speed
				for (unsigned int id = aId; id != 0; id = Database::backlink.Get(id))
				{
					if (Database::setspeed.Find(id))
					{
						Apply(id, -Database::engineaddtemplate.Get(aId));
						break;
					}
				}
			}
		}
		engineaddinitializer;
	}
}
