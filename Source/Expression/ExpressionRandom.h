#pragma once

#include "Expression.h"
#include "ExpressionOperator.h"
#include "ExpressionSchema.h"

//
// RANDOM EXPRESSION
// returns a random value
//

// TO DO: float[width] random

// configure typed random
template <typename T> void ConfigureRandom(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
