#pragma once

#include "Expression.h"

//
// EXTEND EXPRESSIONS
// extend a scalar type to a vector type
//

namespace Expression
{
	template <typename T, typename A> T Extend(Context &aContext);
	template <> GAME_API __m128 Extend<__m128, float>(Context &aContext);
}