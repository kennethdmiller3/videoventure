#include "StdAfx.h"

#include "ExpressionVariable.h"
#include "ExpressionConfigure.h"

// evaluate boolean variable
template <> bool EvaluateVariable(EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	return aContext.mVars->Get(name) != 0.0f;
}

// evaluate scalar variable
template <> float EvaluateVariable(EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	return aContext.mVars->Get(name);
}

// evaluate vector variable
template <> __m128 EvaluateVariable(EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	__m128 ret = _mm_setzero_ps();
	for (register int i = 0; i < 4; ++i)
		ret.m128_f32[i] = aContext.mVars->Get(name+i);
	return ret;
}

static Expression::Loader<bool> variablebool(0x19385305 /* "variable" */, ConfigureVariable<bool>);
static Expression::Loader<float> variablefloat(0x19385305 /* "variable" */, ConfigureVariable<float>);
static Expression::Loader<__m128> variablevector(0x19385305 /* "variable" */, ConfigureVariable<__m128>);
