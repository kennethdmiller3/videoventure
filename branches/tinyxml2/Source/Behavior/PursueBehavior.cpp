#include "StdAfx.h"

#include "PursueBehavior.h"
#include "TargetBehavior.h"
#include "BotUtilities.h"
#include "Controller.h"
#include "Entity.h"

namespace Database
{
	Typed<PursueBehaviorTemplate> pursuebehaviortemplate(0xb9b0800f /* "pursuebehaviortemplate" */);
	Typed<PursueBehavior *> pursuebehavior(0xa211bfc9 /* "pursuebehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		class PursueBehaviorLoader
		{
		public:
			PursueBehaviorLoader()
			{
				AddConfigure(0x0297228f /* "pursue" */, Entry(this, &PursueBehaviorLoader::Configure));
			}

			unsigned int Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				PursueBehaviorTemplate &pursue = Database::pursuebehaviortemplate.Open(aId);
				pursue.Configure(element, aId);
				Database::pursuebehaviortemplate.Close(aId);
				return 0xb9b0800f /* "pursuebehaviortemplate" */;
			}
		}
		pursuebehaviorloader;
	}

	namespace Initializer
	{
		class PursueBehaviorInitializer
		{
		public:
			PursueBehaviorInitializer()
			{
				AddActivate(0xb9b0800f /* "pursuebehaviortemplate" */, ActivateEntry(this, &PursueBehaviorInitializer::Activate));
				AddDeactivate(0xb9b0800f /* "pursuebehaviortemplate" */, DeactivateEntry(this, &PursueBehaviorInitializer::Deactivate));
			}

			Behavior *Activate(unsigned int aId, Controller *aController)
			{
				const PursueBehaviorTemplate &pursuebehaviortemplate = Database::pursuebehaviortemplate.Get(aId);
				PursueBehavior *pursuebehavior = new PursueBehavior(aId, pursuebehaviortemplate, aController);
				Database::pursuebehavior.Put(aId, pursuebehavior);
				return pursuebehavior;
			}

			void Deactivate(unsigned int aId)
			{
				if (PursueBehavior *pursuebehavior = Database::pursuebehavior.Get(aId))
				{
					delete pursuebehavior;
					Database::pursuebehavior.Delete(aId);
				}
			}
		}
		pursuebehaviorinitializer;
	}
}

PursueBehaviorTemplate::PursueBehaviorTemplate()
: mStrength(0.0f)
, mLeading(0.0f)
, mOffset(Transform2::Identity())
{
}

bool PursueBehaviorTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	element->QueryFloatAttribute("leading", &mLeading);
	if (element->QueryFloatAttribute("angle", &mOffset.a) == tinyxml2::XML_SUCCESS)
		mOffset.a *= float(M_PI) / 180.0f;
	element->QueryFloatAttribute("x", &mOffset.p.x);
	element->QueryFloatAttribute("y", &mOffset.p.y);
	return true;
}

PursueBehavior::PursueBehavior(unsigned int aId, const PursueBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &PursueBehavior::Execute);
}

// pursue behavior
Status PursueBehavior::Execute(void)
{
	// get target
	const TargetData &targetdata = Database::targetdata.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(targetdata.mTarget);
	if (!targetEntity)
		return runningTask;

	// get pursue behavior template
	const PursueBehaviorTemplate &pursue = Database::pursuebehaviortemplate.Get(mId);

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// direction to target
	Vector2 targetDir(pursue.mOffset.Untransform(TargetDir(pursue.mLeading, entity, targetEntity, targetdata.mOffset)));
	
	// save range
	float distSq = targetDir.LengthSq();

	// normalize direction
	targetDir *= InvSqrt(distSq);

	// move towards target
	mController->mMove += pursue.mStrength * targetDir;

	return runningTask;
}
