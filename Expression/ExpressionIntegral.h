#pragma once

struct EntityContext;

// evaluate numerical integration
float EvaluateIntegral(EntityContext &aContext);

// evaluate numerical differentiation
float EvaluateDifferential(EntityContext &aContext);
