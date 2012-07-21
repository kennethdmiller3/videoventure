#pragma once

struct EntityContext;

//
// TIME EXPRESSION
//

// world time
GAME_API float EvaluateWorldTime(Expression::Context &aContext);

// entity-local time
GAME_API float EvaluateTime(EntityContext &aContext);
