#pragma once

#include "ExpressionOperator.h"

//
// SIMD-SPECIALIZED OPERATOR EXPRESSIONS
//

namespace Expression
{
	//
	// ARITMETIC OPERATORS
	//

	// add
	template <> __m128 Add(Context &aContext);

	// subtract
	template <> __m128 Sub(Context &aContext);

	// multiply
	template <> __m128 Mul(Context &aContext);

	// divide
	template <> __m128 Div(Context &aContext);

	// reverse divide
	template <> __m128 DivR(Context &aContext);

	// negate (unary minus)
	template <> __m128 Neg(Context &aContext);

	// reciprocal
	template <> __m128 Rcp(Context &aContext);

	// increment
	template <> __m128 Inc(Context &aContext);

	// decrement
	template <> __m128 Dec(Context &aContext);


	//
	// EXPONENTIAL FUNCTIONS
	//

	// square root
	template <> __m128 Sqrt(Context &aContext);

	// reciprocal square root
	template <> __m128 InvSqrt(Context &aContext);


	//
	// COMMON FUNCTIONS
	//

	// absolute value
	template <> __m128 Abs<__m128>(Context &aContext);

	// minimum
	template <> __m128 Min(Context &aContext);

	// maximum
	template <> __m128 Max(Context &aContext);

	// clamp
	template <> __m128 Clamp(Context &aContext);

	// linear interpolate
	template <> __m128 Lerp(Context &aContext);
}
