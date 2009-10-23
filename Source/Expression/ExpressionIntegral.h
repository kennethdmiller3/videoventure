#pragma once

struct EntityContext;

// evaluate numerical integration
GAME_API float EvaluateIntegral(EntityContext &aContext);

// evaluate numerical differentiation
GAME_API float EvaluateDifferential(EntityContext &aContext);
