#pragma once

struct EntityContext;

//
// TIME EXPRESSION
//

// world time
float EvaluateWorldTime(Expression::Context &aContext);

// entity-local time
float EvaluateTime(EntityContext &aContext);
