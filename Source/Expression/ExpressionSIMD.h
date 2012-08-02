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
		return _mm_set_ps(aOp(), aOp(), aOp(), aOp());
	};

	// componentwise unary operator adapter
	template <> inline __m128 ComponentUnary<__m128>(Context &aContext, float (*aOp)(float))
	{
		const __m128 arg1(Expression::Evaluate<__m128>(aContext));
		return _mm_set_ps(
			aOp(reinterpret_cast<const float *>(&arg1)[3]),
			aOp(reinterpret_cast<const float *>(&arg1)[2]),
			aOp(reinterpret_cast<const float *>(&arg1)[1]),
			aOp(reinterpret_cast<const float *>(&arg1)[0])
			);
	}

	// componentwise unary operator adapter
	template <> inline __m128 ComponentBinary<__m128>(Context &aContext, float (*aOp)(float, float))
	{
		const __m128 arg1(Expression::Evaluate<__m128>(aContext));
		const __m128 arg2(Expression::Evaluate<__m128>(aContext));
		return _mm_set_ps(
			aOp(reinterpret_cast<const float *>(&arg1)[3], reinterpret_cast<const float *>(&arg2)[3]),
			aOp(reinterpret_cast<const float *>(&arg1)[2], reinterpret_cast<const float *>(&arg2)[2]),
			aOp(reinterpret_cast<const float *>(&arg1)[1], reinterpret_cast<const float *>(&arg2)[1]),
			aOp(reinterpret_cast<const float *>(&arg1)[0], reinterpret_cast<const float *>(&arg2)[0])
		);
	}

	// componentwise ternary operator adapter
	template <> inline __m128 ComponentTernary<__m128>(Context &aContext, float (*aOp)(float, float, float))
	{
		const __m128 arg1(Expression::Evaluate<__m128>(aContext));
		const __m128 arg2(Expression::Evaluate<__m128>(aContext));
		const __m128 arg3(Expression::Evaluate<__m128>(aContext));
		return _mm_set_ps(
			aOp(reinterpret_cast<const float *>(&arg1)[3], reinterpret_cast<const float *>(&arg2)[3], reinterpret_cast<const float *>(&arg3)[3]),
			aOp(reinterpret_cast<const float *>(&arg1)[2], reinterpret_cast<const float *>(&arg2)[2], reinterpret_cast<const float *>(&arg3)[2]),
			aOp(reinterpret_cast<const float *>(&arg1)[1], reinterpret_cast<const float *>(&arg2)[1], reinterpret_cast<const float *>(&arg3)[1]),
			aOp(reinterpret_cast<const float *>(&arg1)[0], reinterpret_cast<const float *>(&arg2)[0], reinterpret_cast<const float *>(&arg3)[0])
		);
	}
}
