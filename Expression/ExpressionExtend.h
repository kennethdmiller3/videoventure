#pragma once

#include "Expression.h"

//
// EXTEND EXPRESSIONS
// extend a scalar type to a vector type
//

namespace Expression
{
	template <typename T, typename A> const T Extend(Context &aContext);
}