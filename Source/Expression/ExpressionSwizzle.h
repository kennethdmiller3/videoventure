#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"

//
// SWIZZLE EXPRESSION
// shuffles components of a vector
//

namespace Expression
{
	// swizzle operators
	template <typename T> T Swizzle(Context &aContext);
	template <> GAME_API __m128 Swizzle<__m128>(Context &aContext);
}

// configure swizzle expression
template <typename T> void ConfigureSwizzle(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
