#include "StdAfx.h"

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
	template <> __m128 Add(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_add_ps(arg1, arg2);
	}

	// subtract
	template <> __m128 Sub(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_sub_ps(arg1, arg2);
	}

	// multiply
	template <> __m128 Mul(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_mul_ps(arg1, arg2);
	}

	// divide
	template <> __m128 Div(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_div_ps(arg1, arg2);
	}

	// reverse divide
	template <> __m128 DivR(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_div_ps(arg2, arg1);
	}

	// negate (unary minus)
	template <> __m128 Neg(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_sub_ps(_mm_setzero_ps(), arg1);
	}

	// reciprocal
	template <> __m128 Rcp(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_rcp_ps(arg1);
	}

	// increment
	template <> __m128 Inc(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_add_ps(arg1, _mm_set_ps1(1));
	}

	// decrement
	template <> __m128 Dec(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_sub_ps(arg1, _mm_set_ps1(1));
	}


	//
	// EXPONENTIAL FUNCTIONS
	//

	// square root
	template <> __m128 Sqrt(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_sqrt_ps(arg1);
	}

	// reciprocal square root
	template <> __m128 InvSqrt(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_rsqrt_ps(arg1);
	}


	//
	// COMMON FUNCTIONS
	//

	// absolute value
	template <> __m128 Abs<__m128>(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_max_ps(arg1, _mm_sub_ps(_mm_setzero_ps(), arg1));
	}

	// minimum
	template <> __m128 Min(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_min_ps(arg1, arg2);
	}

	// maximum
	template <> __m128 Max(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_max_ps(arg1, arg2);
	}

	// clamp
	template <> __m128 Clamp(Context &aContext)
	{
		__m128 val(Evaluate<__m128>(aContext));
		__m128 min(Evaluate<__m128>(aContext));
		__m128 max(Evaluate<__m128>(aContext));
		return _mm_min_ps(_mm_max_ps(val, min), max);
	}

	// linear interpolate
	template <> __m128 Lerp(Context &aContext)
	{
		__m128 v0(Evaluate<__m128>(aContext));
		__m128 v1(Evaluate<__m128>(aContext));
		float s(Evaluate<float>(aContext));
		return _mm_add_ps(v0, _mm_mul_ps(_mm_sub_ps(v1, v0), _mm_set_ps1(s)));
	}
}
