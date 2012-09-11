#pragma once

#include "Expression.h"

//
// EXTEND EXPRESSIONS
// extend a scalar type to a vector type
//

namespace Expression
{
	GAME_API __m128 Extend(Context &aContext);
}