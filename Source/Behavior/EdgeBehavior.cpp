#include "StdAfx.h"

#include "EdgeBehavior.h"
#include "..\Controller.h"
#include "..\Entity.h"
#include "..\Collidable.h"


namespace Database
{
	Typed<EdgeBehaviorTemplate> edgebehaviortemplate(0xae05bb20 /* "edgebehaviortemplate" */);
}

EdgeBehaviorTemplate::EdgeBehaviorTemplate()
: mStrength(1.0f/64.0f)
, mDistance(64.0f)
{
}

bool EdgeBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	element->QueryFloatAttribute("distance", &mDistance);

	return true;
}

EdgeBehavior::EdgeBehavior(unsigned int aId, Controller *aController)
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
	b2AABB edge(Collidable::GetBoundary());
	edge.lowerBound.x += edgebehavior.mDistance;
	edge.upperBound.x -= edgebehavior.mDistance;
	edge.lowerBound.y += edgebehavior.mDistance;
	edge.upperBound.y -= edgebehavior.mDistance;
	Vector2 push(0, 0);
	if (transform.p.x < edge.lowerBound.x)
		push.x += (edge.lowerBound.x - transform.p.x) * edgebehavior.mStrength;
	if (transform.p.x > edge.upperBound.x)
		push.x += (edge.upperBound.x - transform.p.x) * edgebehavior.mStrength;
	if (transform.p.y < edge.lowerBound.y)
		push.y += (edge.lowerBound.y - transform.p.y) * edgebehavior.mStrength;
	if (transform.p.y > edge.upperBound.y)
		push.y += (edge.upperBound.y - transform.p.y) * edgebehavior.mStrength;
	if (push.x || push.y)
	{
		mController->mMove += push;
		push = transform.Unrotate(push);
		mController->mTurn += push.x < 0 ? push.Length() : -push.Length();
	}

	return runningTask;
}