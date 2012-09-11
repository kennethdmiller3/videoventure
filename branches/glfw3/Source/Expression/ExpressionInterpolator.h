#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionConfigure.h"
#include "ExpressionTime.h"
#include "Interpolator.h"

//
// INTERPOLATOR EXPRESSION
// returns interpolated value based on parameter
//

//#define EVALUATE_INTERPOLATOR_USE_HINT

// evaluate typed keyframe interpolator
template <typename T> T EvaluateInterpolator(EntityContext &aContext);

// evaluate typed keyframe constant
template <typename T> T EvaluateInterpolatorConstant(EntityContext &aContext);

// configure typed interpolator
template <typename T> void ConfigureInterpolator(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
