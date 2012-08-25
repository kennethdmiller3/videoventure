#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"

//
// CONSTRUCT EXPRESSION
// build a vector from scalar components
//

namespace Expression
{
	// constructors
	template <> GAME_API bool Construct<bool, float>(Context &aContext);
	template <> GAME_API __m128 Construct<__m128, float>(Context &aContext);
}

// configure construct expression
template <typename T> void ConfigureConstruct(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
