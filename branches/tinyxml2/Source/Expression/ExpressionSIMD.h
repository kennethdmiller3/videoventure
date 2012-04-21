#pragma once

//
// SPECIALIZATIONS FOR 128-BIT SIMD TYPE
//

namespace Expression
{
	// read unaligned value from an expression stream
	template <> inline const __m128 Read<__m128>(Context &aContext)
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

	// nullary operator adapter
	template <> struct ComponentNullary<__m128, 4>
	{
		// requres that T support operator[]
		template <typename OR, OR Op()> static const __m128 Evaluate(Context &aContext)
		{
			return _mm_set_ps(Op(), Op(), Op(), Op());
		}
	};

	// unary operator adapter
	template <> struct ComponentUnary<__m128, 4>
	{
		template <typename OR, typename O1, OR Op(O1)> static const __m128 Evaluate(Context &aContext)
		{
			const __m128 arg1(Expression::Evaluate<__m128>(aContext));
			return _mm_set_ps(
				Op(reinterpret_cast<const float *>(&arg1)[3]),
				Op(reinterpret_cast<const float *>(&arg1)[2]),
				Op(reinterpret_cast<const float *>(&arg1)[1]),
				Op(reinterpret_cast<const float *>(&arg1)[0])
				);
		}
	};

	// binary operator adapter
	template <> struct ComponentBinary<__m128, 4>
	{
		template <typename OR, typename O1, typename O2, OR Op(O1, O2)> static const __m128 Evaluate(Context &aContext)
		{
			const __m128 arg1(Expression::Evaluate<__m128>(aContext));
			const __m128 arg2(Expression::Evaluate<__m128>(aContext));
			return _mm_set_ps(
				Op(reinterpret_cast<const float *>(&arg1)[3], reinterpret_cast<const float *>(&arg2)[3]),
				Op(reinterpret_cast<const float *>(&arg1)[2], reinterpret_cast<const float *>(&arg2)[2]),
				Op(reinterpret_cast<const float *>(&arg1)[1], reinterpret_cast<const float *>(&arg2)[1]),
				Op(reinterpret_cast<const float *>(&arg1)[0], reinterpret_cast<const float *>(&arg2)[0])
				);
		}
	};

	// ternary operator adapter
	template <> struct ComponentTernary<__m128, 4>
	{
		// requres that T support operator[]
		template <typename OR, typename O1, typename O2, typename O3, OR Op(O1, O2, O3)> static const __m128 Evaluate(Context &aContext)
		{
			const __m128 arg1(Expression::Evaluate<__m128>(aContext));
			const __m128 arg2(Expression::Evaluate<__m128>(aContext));
			const __m128 arg3(Expression::Evaluate<__m128>(aContext));
			return _mm_set_ps(
				Op(reinterpret_cast<const float *>(&arg1)[3], reinterpret_cast<const float *>(&arg2)[0], reinterpret_cast<const float *>(&arg3)[3]),
				Op(reinterpret_cast<const float *>(&arg1)[2], reinterpret_cast<const float *>(&arg2)[1], reinterpret_cast<const float *>(&arg3)[2]),
				Op(reinterpret_cast<const float *>(&arg1)[1], reinterpret_cast<const float *>(&arg2)[2], reinterpret_cast<const float *>(&arg3)[1]),
				Op(reinterpret_cast<const float *>(&arg1)[0], reinterpret_cast<const float *>(&arg2)[3], reinterpret_cast<const float *>(&arg3)[0])
				);
		}
	};
}
