#include "StdAfx.h"

#include "Expression.h"
#include "ExpressionTime.h"
#include "ExpressionEntity.h"

//
// TIME EXPRESSION
//

// world time
float EvaluateWorldTime(Expression::Context &aContext)
{
	return (sim_turn + sim_fraction) / sim_rate;
}

// entity local time
float EvaluateTime(EntityContext &aContext)
{
	return aContext.mParam;
}
