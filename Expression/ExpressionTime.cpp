#include "StdAfx.h"

#include "ExpressionTime.h"
#include "ExpressionEntity.h"

//
// TIME EXPRESSION
// returns the entity context time
//

float EvaluateTime(EntityContext &aContext)
{
	return aContext.mParam;
}
