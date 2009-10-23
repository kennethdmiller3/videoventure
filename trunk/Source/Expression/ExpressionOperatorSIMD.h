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
	template <> GAME_API __m128 Add(Context &aContext);

	// subtract
	template <> GAME_API __m128 Sub(Context &aContext);

	// multiply
	template <> GAME_API __m128 Mul(Context &aContext);

	// divide
	template <> GAME_API __m128 Div(Context &aContext);

	// reverse divide
	template <> GAME_API __m128 DivR(Context &aContext);

	// negate (unary minus)
	template <> GAME_API __m128 Neg(Context &aContext);

	// reciprocal
	template <> GAME_API __m128 Rcp(Context &aContext);

	// increment
	template <> GAME_API __m128 Inc(Context &aContext);

	// decrement
	template <> GAME_API __m128 Dec(Context &aContext);


	//
	// EXPONENTIAL FUNCTIONS
	//

	// square root
	template <> GAME_API __m128 Sqrt(Context &aContext);

	// reciprocal square root
	template <> GAME_API __m128 InvSqrt(Context &aContext);


	//
	// COMMON FUNCTIONS
	//

	// absolute value
	template <> GAME_API __m128 Abs<__m128>(Context &aContext);

	// minimum
	template <> GAME_API __m128 Min(Context &aContext);

	// maximum
	template <> GAME_API __m128 Max(Context &aContext);

	// clamp
	template <> GAME_API __m128 Clamp(Context &aContext);

	// linear interpolate
	template <> GAME_API __m128 Lerp(Context &aContext);
}
