#include "StdAfx.h"

#include "ExpressionIntegral.h"
#include "ExpressionEntity.h"

// evaluate integral
float EvaluateIntegral(EntityContext &aContext)
{
	Database::Key key((aContext.mStream - aContext.mBegin)*4);
	float &prev_param = aContext.mVars->Open(key);
	float &prev_value = aContext.mVars->Open(key+1);
	float param(Expression::Evaluate<float>(aContext));
	float value(Expression::Evaluate<float>(aContext));
	float result = prev_value + value * (param - prev_param);
	prev_param = param;
	prev_value = result;
	aContext.mVars->Close(key);
	aContext.mVars->Close(key+1);
	aContext.mVars->Close(key+2);
	return result;
}

// evaluate differential
float EvaluateDifferential(EntityContext &aContext)
{
	Database::Key key((aContext.mStream - aContext.mBegin)*4);
	float &prev_param = aContext.mVars->Open(key);
	float &prev_value = aContext.mVars->Open(key+1);
	float param(Expression::Evaluate<float>(aContext));
	float value(Expression::Evaluate<float>(aContext));
	float result = (aContext.mParam != prev_param) ? (value - prev_value) / (param - prev_param) : 0.0f;
	prev_value = value;
	prev_param = param;
	aContext.mVars->Close(key);
	aContext.mVars->Close(key+1);
	return result;
}


