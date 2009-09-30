#include "StdAfx.h"

#include "ExpressionEntity.h"

#include "Drawlist.h"	// <-- for variable

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
