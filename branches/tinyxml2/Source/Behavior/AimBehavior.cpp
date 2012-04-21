#include "StdAfx.h"

#include "AimBehavior.h"
#include "TargetBehavior.h"
#include "BotUtilities.h"
#include "Controller.h"
#include "Entity.h"

namespace Database
{
	Typed<AimBehaviorTemplate> aimbehaviortemplate(0x48ae5f7a /* "aimbehaviortemplate" */);
	Typed<Typed<FireConeTemplate> > fireconetemplate(0x00dbebf8 /* "fireconetemplate" */);
	Typed<AimBehavior *> aimbehavior(0xb960a4d0 /* "aimbehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		class AimBehaviorLoader
		{
		public:
			AimBehaviorLoader()
			{
				AddConfigure(0x383251f6 /* "aim" */, Entry(this, &AimBehaviorLoader::Configure));
			}

			unsigned int Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				AimBehaviorTemplate &aim = Database::aimbehaviortemplate.Open(aId);
				aim.Configure(element, aId);
				Database::aimbehaviortemplate.Close(aId);
				return 0x48ae5f7a /* "aimbehaviortemplate" */;
			}
		}
		aimbehaviorloader;

		class FireConeLoader
		{
		public:
			FireConeLoader()
			{
				AddConfigure(0x8eab16d9 /* "fire" */, Entry(this, &FireConeLoader::Configure));
			}

			unsigned int Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				Database::Typed<FireConeTemplate> &firebehaviors = Database::fireconetemplate.Open(aId);
				const char *name = element->Attribute("name");
				unsigned int aSubId = name ? Hash(name) : firebehaviors.GetCount() + 1;
				FireConeTemplate &firebehavior = firebehaviors.Open(aSubId);
				firebehavior.Configure(element, aId);
				firebehaviors.Close(aSubId);
				Database::fireconetemplate.Close(aId);
				return 0x00dbebf8 /* "fireconetemplate" */;
			}
		}
		fireconeloader;
	}

	namespace Initializer
	{
		class AimBehaviorInitializer
		{
		public:
			AimBehaviorInitializer()
			{
				AddActivate(0x48ae5f7a /* "aimbehaviortemplate" */, ActivateEntry(this, &AimBehaviorInitializer::Activate));
				AddDeactivate(0x48ae5f7a /* "aimbehaviortemplate" */, DeactivateEntry(this, &AimBehaviorInitializer::Deactivate));
				AddActivate(0x00dbebf8 /* "fireconetemplate" */, ActivateEntry(this, &AimBehaviorInitializer::Activate));
				AddDeactivate(0x00dbebf8 /* "fireconetemplate" */, DeactivateEntry(this, &AimBehaviorInitializer::Deactivate));
			}

			Behavior *Activate(unsigned int aId, Controller *aController)
			{
				const AimBehaviorTemplate &aimbehaviortemplate = Database::aimbehaviortemplate.Get(aId);
				if (AimBehavior *aimbehavior = Database::aimbehavior.Get(aId))
					return aimbehavior;
				AimBehavior *aimbehavior = new AimBehavior(aId, aimbehaviortemplate, aController);
				Database::aimbehavior.Put(aId, aimbehavior);
				return aimbehavior;
			}

			void Deactivate(unsigned int aId)
			{
				if (AimBehavior *aimbehavior = Database::aimbehavior.Get(aId))
				{
					delete aimbehavior;
					Database::aimbehavior.Delete(aId);
				}
			}
		}
		aimbehaviorinitializer;
	}
}

FireConeTemplate::FireConeTemplate()
: mRange(0.0f)
, mDirection(0.0f)
, mAngle(0.3f)
, mChannel(-1)
{
}

bool FireConeTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	if (element->QueryIntAttribute("channel", &mChannel) == tinyxml2::XML_SUCCESS)
		--mChannel;
	element->QueryFloatAttribute("range", &mRange);
	if (element->QueryFloatAttribute("direction", &mDirection) == tinyxml2::XML_SUCCESS)
		mDirection *= float(M_PI) / 180.0f;
	if (element->QueryFloatAttribute("angle", &mAngle) == tinyxml2::XML_SUCCESS)
		mAngle *= float(M_PI) / 180.0f;
	return true;
}


AimBehaviorTemplate::AimBehaviorTemplate()
: mStrength(0.0f)
, mLeading(0.0f)
{
}

bool AimBehaviorTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	element->QueryFloatAttribute("leading", &mLeading);
	return true;
}

AimBehavior::AimBehavior(unsigned int aId, const AimBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &AimBehavior::Execute);
}

// aim behavior
Status AimBehavior::Execute(void)
{
	// get target
	const TargetData &targetdata = Database::targetdata.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(targetdata.mTarget);
	if (!targetEntity)
		return runningTask;

	// get aim behavior template
	const AimBehaviorTemplate &aim = Database::aimbehaviortemplate.Get(mId);

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// direction to target
	Vector2 targetDir(TargetDir(aim.mLeading, entity, targetEntity, targetdata.mOffset));

	// save range
	float distSq = targetDir.LengthSq();

	// normalize direction
	targetDir *= InvSqrt(distSq);

	// local direction
	mController->mAim = transform.Unrotate(targetDir);

	// if aiming...
	if (aim.mStrength != 0)
	{
		// turn towards target direction
		mController->mTurn += aim.mStrength * SteerTowards(entity, mController->mAim);
	}

	// for each fire cone...
	for (Database::Typed<FireConeTemplate>::Iterator itor(Database::fireconetemplate.Find(mId)); itor.IsValid(); ++itor)
	{
		// get the fire cone properties
		const FireConeTemplate &fire = itor.GetValue();

		// if not set to fire and target is in range...
		if (!mController->mFire[fire.mChannel] &&
			distSq <= fire.mRange * fire.mRange)
		{
			// normalize direction
			targetDir *= InvSqrt(distSq);

			// local direction
			Vector2 localDir = transform.Unrotate(targetDir);

			// angle to target
			float aimAngle = -atan2f(localDir.x, localDir.y);

			float localAngle = aimAngle - fire.mDirection;
			if (localAngle > float(M_PI))
				localAngle -= float(M_PI)*2.0f;
			else if (localAngle < -float(M_PI))
				localAngle += float(M_PI)*2.0f;
			if (fabsf(localAngle) <= fire.mAngle)
				mController->mFire[fire.mChannel] = sqrtf(distSq) / fire.mRange; // encode range into fire (HACK)
		}
	}

	return runningTask;
}
