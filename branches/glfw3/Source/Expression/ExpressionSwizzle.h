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
	GAME_API __m128 Swizzle(Context &aContext);
}

// configure swizzle expression
void ConfigureSwizzle(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
