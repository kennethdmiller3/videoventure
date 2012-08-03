#pragma once

//
// SPECIALIZATIONS FOR 128-BIT SIMD TYPE
//

namespace Expression
{
	// read unaligned value from an expression stream
	template <> inline __m128 Read<__m128>(Context &aContext)
	{
		const __m128 value(_mm_loadu_ps(reinterpret_cast<const float *>(aContext.mStream)));
		aContext.mStream += (sizeof(__m128) + sizeof(unsigned int) - 1) / sizeof(unsigned int);
		return value;
	}

	// allocate object from a buffer
	template <> inline __m128 *New<__m128, __m128>(std::vector<unsigned int> &aBuffer, __m128 aArg1)
	{
		void *ptr = Alloc(aBuffer, sizeof(__m128));
		_mm_storeu_ps(reinterpret_cast<float *>(ptr), aArg1);
		return reinterpret_cast<__m128 *>(ptr);
	}

	// componentwise nullary operator adapter
	template <> inline __m128 ComponentNullary<__m128>(Context &aContext, float (*aOp)())
	{
		__m128 ret = _mm_setzero_ps();
		for (register int i = 0; i < 4; ++i)
			ret.m128_f32[i] = aOp();
		return ret;
	};

	// componentwise unary operator adapter
	template <> inline __m128 ComponentUnary<__m128>(Context &aContext, float (*aOp)(float))
	{
		const __m128 arg1(Expression::Evaluate<__m128>(aContext));
		__m128 ret = _mm_setzero_ps();
		for (register int i = 0; i < 4; ++i)
			ret.m128_f32[i] = aOp(arg1.m128_f32[i]);
		return ret;
	}

	// componentwise binary operator adapter
	template <> inline __m128 ComponentBinary<__m128>(Context &aContext, float (*aOp)(float, float))
	{
		const __m128 arg1(Expression::Evaluate<__m128>(aContext));
		const __m128 arg2(Expression::Evaluate<__m128>(aContext));
		__m128 ret = _mm_setzero_ps();
		for (register int i = 0; i < 4; ++i)
			ret.m128_f32[i] = aOp(arg1.m128_f32[i], arg2.m128_f32[i]);
		return ret;
	}

	// componentwise ternary operator adapter
	template <> inline __m128 ComponentTernary<__m128>(Context &aContext, float (*aOp)(float, float, float))
	{
		const __m128 arg1(Expression::Evaluate<__m128>(aContext));
		const __m128 arg2(Expression::Evaluate<__m128>(aContext));
		const __m128 arg3(Expression::Evaluate<__m128>(aContext));
		__m128 ret = _mm_setzero_ps();
		for (register int i = 0; i < 4; ++i)
			ret.m128_f32[i] = aOp(arg1.m128_f32[i], arg2.m128_f32[i], arg3.m128_f32[i]);
		return ret;
	}
}
