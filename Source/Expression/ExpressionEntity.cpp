#include "StdAfx.h"

#include "ExpressionEntity.h"
#include "Entity.h"
#include "Variable.h"

EntityContext::EntityContext(const unsigned int *aBuffer, const size_t aSize, float aParam, unsigned int aId, Database::Typed<float> *aVars)
: Expression::Context(aBuffer)
, mBegin(aBuffer)
, mEnd(aBuffer + aSize)
, mParam(aParam)
, mId(aId)
, mVars(aVars)
{
	if (mVars)
	{
		mOwned = false;
	}
	else
	{
		mVars = &Database::variable.Open(mId);
		mOwned = true;
	}
}

EntityContext::~EntityContext()
{
	if (mOwned)
	{
		Database::variable.Close(mId);
	}
}

__m128 Expression::EvaluatePosition(EntityContext &aContext)
{
	if (const Entity *entity = Database::entity.Get(aContext.mId))
	{
		const Transform2 transform = entity->GetInterpolatedTransform(sim_fraction);
		return _mm_setr_ps(transform.p.x, transform.p.y, transform.a, 1.0f);
	}
	else
	{
		return _mm_setzero_ps();
	}
}

__m128 Expression::EvaluateVelocity(EntityContext &aContext)
{
	if (const Entity *entity = Database::entity.Get(aContext.mId))
	{
		return _mm_setr_ps(entity->GetVelocity().x, entity->GetVelocity().y, entity->GetOmega(), 0.0f);
	}
	else
	{
		return _mm_setzero_ps();
	}
}
