#include "StdAfx.h"

#include "EvadeBehavior.h"
#include "TargetBehavior.h"
#include "Entity.h"
#include "Controller.h"


namespace Database
{
	Typed<EvadeBehaviorTemplate> evadebehaviortemplate(0xe0f0e10a /* "evadebehaviortemplate" */);
	Typed<EvadeBehavior *> evadebehavior(0x5cc74300 /* "evadebehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		class EvadeBehaviorLoader
		{
		public:
			EvadeBehaviorLoader()
			{
				AddConfigure(0x3cf27f66 /* "evade" */, Entry(this, &EvadeBehaviorLoader::Configure));
			}

			unsigned int Configure(unsigned int aId, const TiXmlElement *element)
			{
				EvadeBehaviorTemplate &evade = Database::evadebehaviortemplate.Open(aId);
				evade.Configure(element, aId);
				Database::evadebehaviortemplate.Close(aId);
				return 0xe0f0e10a /* "evadebehaviortemplate" */;
			}
		}
		evadebehaviorloader;
	}

	namespace Initializer
	{
		class EvadeBehaviorInitializer
		{
		public:
			EvadeBehaviorInitializer()
			{
				AddActivate(0xe0f0e10a /* "evadebehaviortemplate" */, ActivateEntry(this, &EvadeBehaviorInitializer::Activate));
				AddDeactivate(0xe0f0e10a /* "evadebehaviortemplate" */, DeactivateEntry(this, &EvadeBehaviorInitializer::Deactivate));
			}

			Behavior *Activate(unsigned int aId, Controller *aController)
			{
				const EvadeBehaviorTemplate &evadebehaviortemplate = Database::evadebehaviortemplate.Get(aId);
				EvadeBehavior *evadebehavior = new EvadeBehavior(aId, evadebehaviortemplate, aController);
				Database::evadebehavior.Put(aId, evadebehavior);
				return evadebehavior;
			}

			void Deactivate(unsigned int aId)
			{
				if (EvadeBehavior *evadebehavior = Database::evadebehavior.Get(aId))
				{
					delete evadebehavior;
					Database::evadebehavior.Delete(aId);
				}
			}
		}
		evadebehaviorinitializer;
	}
}


EvadeBehaviorTemplate::EvadeBehaviorTemplate()
: mStrength(0.0f)
{
}

bool EvadeBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	return true;
}

EvadeBehavior::EvadeBehavior(unsigned int aId, const EvadeBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &EvadeBehavior::Execute);
}

Status EvadeBehavior::Execute(void)
{
	// get target
	const TargetData &targetdata = Database::targetdata.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(targetdata.mTarget);
	if (!targetEntity)
		return runningTask;

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// get evade behavior template
	const EvadeBehaviorTemplate &evade = Database::evadebehaviortemplate.Get(mId);

	// target entity transform
	const Transform2 &targetTransform = targetEntity->GetTransform();

	// evade target's front vector
	Vector2 local(targetTransform.Untransform(entity->GetPosition()));
	if (local.y > 0)
	{
		local *= InvSqrt(local.LengthSq());
		float dir = local.x > 0 ? 1.0f : -1.0f;
		mController->mMove += evade.mStrength * dir * local.y * local.y * local.y * targetTransform.Rotate(Vector2(local.y, -local.x));
	}

	return runningTask;
}
