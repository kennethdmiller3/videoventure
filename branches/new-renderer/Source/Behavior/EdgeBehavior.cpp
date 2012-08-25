#include "StdAfx.h"

#include "EdgeBehavior.h"
#include "Controller.h"
#include "Entity.h"
#include "Collidable.h"


namespace Database
{
	Typed<EdgeBehaviorTemplate> edgebehaviortemplate(0xae05bb20 /* "edgebehaviortemplate" */);
	Typed<EdgeBehavior *> edgebehavior(0x7be01fce /* "edgebehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		static unsigned int EdgeBehaviorConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			EdgeBehaviorTemplate &edge = Database::edgebehaviortemplate.Open(aId);
			edge.Configure(element, aId);
			Database::edgebehaviortemplate.Close(aId);
			return 0xae05bb20 /* "edgebehaviortemplate" */;
		}
		Configure edgebehaviorconfigure(0x56f6d83c /* "edge" */, EdgeBehaviorConfigure);
	}

	namespace Initializer
	{
		static Behavior *EdgeBehaviorActivate(unsigned int aId, Controller *aController)
		{
			const EdgeBehaviorTemplate &edgebehaviortemplate = Database::edgebehaviortemplate.Get(aId);
			EdgeBehavior *edgebehavior = new EdgeBehavior(aId, edgebehaviortemplate, aController);
			Database::edgebehavior.Put(aId, edgebehavior);
			return edgebehavior;
		}
		Activate edgebehavioractivate(0xae05bb20 /* "edgebehaviortemplate" */, EdgeBehaviorActivate);

		static void EdgeBehaviorDeactivate(unsigned int aId)
		{
			if (EdgeBehavior *edgebehavior = Database::edgebehavior.Get(aId))
			{
				delete edgebehavior;
				Database::edgebehavior.Delete(aId);
			}
		}
		Deactivate edgebehaviordeactivate(0xae05bb20 /* "edgebehaviortemplate" */, EdgeBehaviorDeactivate);
	}
}

EdgeBehaviorTemplate::EdgeBehaviorTemplate()
: mStrength(1.0f/64.0f)
, mDistance(64.0f)
{
}

bool EdgeBehaviorTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	element->QueryFloatAttribute("distance", &mDistance);

	return true;
}

EdgeBehavior::EdgeBehavior(unsigned int aId, const EdgeBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &EdgeBehavior::Execute);
}

Status EdgeBehavior::Execute(void)
{
	// get edge behavior template
	const EdgeBehaviorTemplate &edgebehavior = Database::edgebehaviortemplate.Get(mController->GetId());

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// push away from the edges of the world (HACK)
	AlignedBox2 edge(Collidable::GetBoundary());
	edge.min.x += edgebehavior.mDistance;
	edge.max.x -= edgebehavior.mDistance;
	edge.min.y += edgebehavior.mDistance;
	edge.max.y -= edgebehavior.mDistance;
	Vector2 push(0, 0);
	if (transform.p.x < edge.min.x)
		push.x += (edge.min.x - transform.p.x) * edgebehavior.mStrength;
	if (transform.p.x > edge.max.x)
		push.x += (edge.max.x - transform.p.x) * edgebehavior.mStrength;
	if (transform.p.y < edge.min.y)
		push.y += (edge.min.y - transform.p.y) * edgebehavior.mStrength;
	if (transform.p.y > edge.max.y)
		push.y += (edge.max.y - transform.p.y) * edgebehavior.mStrength;
	if (push.x || push.y)
	{
		mController->mMove += push;
		push = transform.Unrotate(push);
		mController->mTurn += push.x < 0 ? push.Length() : -push.Length();
	}

	return runningTask;
}