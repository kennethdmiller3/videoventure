#include "StdAfx.h"

#include "WanderBehavior.h"
#include "Controller.h"

namespace Database
{
	Typed<WanderBehaviorTemplate> wanderbehaviortemplate(0xd1cc22c8 /* "wanderbehaviortemplate" */);
	Typed<WanderBehavior *> wanderbehavior(0xa1410cd6 /* "wanderbehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		class WanderBehaviorLoader
		{
		public:
			WanderBehaviorLoader()
			{
				AddConfigure(0xf23b7114 /* "wander" */, Entry(this, &WanderBehaviorLoader::Configure));
			}

			unsigned int Configure(unsigned int aId, const TiXmlElement *element)
			{
				WanderBehaviorTemplate &wander = Database::wanderbehaviortemplate.Open(aId);
				wander.Configure(element, aId);
				Database::wanderbehaviortemplate.Close(aId);
				return 0xd1cc22c8 /* "wanderbehaviortemplate" */;
			}
		}
		wanderbehaviorloader;
	}

	namespace Initializer
	{
		class WanderBehaviorInitializer
		{
		public:
			WanderBehaviorInitializer()
			{
				AddActivate(0xd1cc22c8 /* "wanderbehaviortemplate" */, ActivateEntry(this, &WanderBehaviorInitializer::Activate));
				AddDeactivate(0xd1cc22c8 /* "wanderbehaviortemplate" */, DeactivateEntry(this, &WanderBehaviorInitializer::Deactivate));
			}

			Behavior *Activate(unsigned int aId, Controller *aController)
			{
				const WanderBehaviorTemplate &wanderbehaviortemplate = Database::wanderbehaviortemplate.Get(aId);
				WanderBehavior *wanderbehavior = new WanderBehavior(aId, wanderbehaviortemplate, aController);
				Database::wanderbehavior.Put(aId, wanderbehavior);
				return wanderbehavior;
			}

			void Deactivate(unsigned int aId)
			{
				if (WanderBehavior *wanderbehavior = Database::wanderbehavior.Get(aId))
				{
					delete wanderbehavior;
					Database::wanderbehavior.Delete(aId);
				}
			}
		}
		wanderbehaviorinitializer;
	}
}

WanderBehaviorTemplate::WanderBehaviorTemplate()
: mSide(0.0f)
, mSideRate(0.0f)
, mFront(0.0f)
, mFrontRate(0.0f)
, mTurn(0.0f)
, mTurnRate(0.0f)
{
}

bool WanderBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("side", &mSide);
	element->QueryFloatAttribute("siderate", &mSideRate);
	element->QueryFloatAttribute("front", &mFront);
	element->QueryFloatAttribute("frontrate", &mFrontRate);
	element->QueryFloatAttribute("turn", &mTurn);
	element->QueryFloatAttribute("turnrate", &mTurnRate);
	return true;
}

WanderBehavior::WanderBehavior(unsigned int aId, const WanderBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
, mSidePhase(Random::Float() * 2.0f * float(M_PI))
, mFrontPhase(Random::Float() * 2.0f * float(M_PI))
, mTurnPhase(Random::Float() * 2.0f * float(M_PI))
{
	bind(this, &WanderBehavior::Execute);
}

Status WanderBehavior::Execute(void)
{
	const WanderBehaviorTemplate &wander = Database::wanderbehaviortemplate.Get(mController->GetId());

	// apply side wander
	if (wander.mSide)
	{
		mController->mMove.x += wander.mSide * sinf(mSidePhase);
		mSidePhase += Random::Float() * wander.mSideRate * 2.0f * float(M_PI) * sim_step;
		if (mSidePhase > 2.0f * float(M_PI))
			mSidePhase -= 2.0f * float(M_PI);
	}

	// apply front wander
	if (wander.mFront)
	{
		mController->mMove.y += wander.mFront * sinf(mFrontPhase);
		mFrontPhase += Random::Float() * wander.mFrontRate * 2.0f * float(M_PI) * sim_step;
		if (mFrontPhase > 2.0f * float(M_PI))
			mFrontPhase -= 2.0f * float(M_PI);
	}

	// apply turn wander
	if (wander.mTurn)
	{
		mController->mTurn += wander.mTurn * sinf(mTurnPhase);
		mTurnPhase += Random::Float() * wander.mTurnRate * 2.0f * float(M_PI) * sim_step;
		if (mTurnPhase > 2.0f * float(M_PI))
			mTurnPhase -= 2.0f * float(M_PI);
	}

	return runningTask;
}