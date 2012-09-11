#pragma once

namespace Expression
{
	// evaluation context
	struct Context
	{
		const unsigned int *mStream;

		Context()
		{
		}

		Context(const unsigned int *aStream)
			: mStream(aStream)
		{
		}
	};

	// allocate data from a buffer
	inline void *Alloc(std::vector<unsigned int> &aBuffer, size_t aSize)
	{
		aSize = (aSize + sizeof(unsigned int) - 1) / sizeof(unsigned int);
		aBuffer.resize(aBuffer.size() + aSize);
		return &aBuffer[aBuffer.size() - aSize];
	}

	// allocate object from a buffer
	template <typename T> T *New(std::vector<unsigned int> &aBuffer)
	{
		return new(Alloc(aBuffer, sizeof(T))) T();
	}
	template <typename T, typename A1> T *New(std::vector<unsigned int> &aBuffer, A1 aArg1)
	{
		return new(Alloc(aBuffer, sizeof(T))) T(aArg1);
	}
	template <typename T, typename A1, typename A2> T *New(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2)
	{
		return new(Alloc(aBuffer, sizeof(T))) T(aArg1, aArg2);
	}
	template <typename T, typename A1, typename A2, typename A3> T *New(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2, A3 aArg3)
	{
		return new(Alloc(aBuffer, sizeof(T))) T(aArg1, aArg2, aArg3);
	}
	template <typename T, typename A1, typename A2, typename A3, typename A4> T *New(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2, A3 aArg3, A4 aArg4)
	{
		return new(Alloc(aBuffer, sizeof(T))) T(aArg1, aArg2, aArg3, aArg4);
	}

	// append an expression to a buffer
	template <typename A1> void Append(std::vector<unsigned int> &aBuffer, A1 aArg1)
	{
		New<A1>(aBuffer, aArg1);
	}
	template <typename A1, typename A2> void Append(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2)
	{
		New<A1>(aBuffer, aArg1);
		New<A2>(aBuffer, aArg2);
	}
	template <typename A1, typename A2, typename A3> void Append(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2, A3 aArg3)
	{
		New<A1>(aBuffer, aArg1);
		New<A2>(aBuffer, aArg2);
		New<A3>(aBuffer, aArg3);
	}
	template <typename A1, typename A2, typename A3, typename A4> void Append(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2, A3 aArg3, A4 aArg4)
	{
		New<A1>(aBuffer, aArg1);
		New<A2>(aBuffer, aArg2);
		New<A3>(aBuffer, aArg3);
		New<A4>(aBuffer, aArg4);
	}
	template <typename A1, typename A2, typename A3, typename A4, typename A5> void Append(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2, A3 aArg3, A4 aArg4, A5 aArg5)
	{
		New<A1>(aBuffer, aArg1);
		New<A2>(aBuffer, aArg2);
		New<A3>(aBuffer, aArg3);
		New<A4>(aBuffer, aArg4);
		New<A5>(aBuffer, aArg5);
	}
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6> void Append(std::vector<unsigned int> &aBuffer, A1 aArg1, A2 aArg2, A3 aArg3, A4 aArg4, A5 aArg5, A6 aArg6)
	{
		New<A1>(aBuffer, aArg1);
		New<A2>(aBuffer, aArg2);
		New<A3>(aBuffer, aArg3);
		New<A4>(aBuffer, aArg4);
		New<A5>(aBuffer, aArg5);
		New<A6>(aBuffer, aArg6);
	}

	// read a value from an expression stream
	template <typename T> T Read(Context &aContext)
	{
		register const T value = *reinterpret_cast<const T *>(aContext.mStream);
		aContext.mStream += (sizeof(T) + sizeof(unsigned int) - 1) / sizeof(unsigned int);
		return value;
	}

	// evaluate an expression stream
	template <typename T> T Evaluate(Context &aContext)
	{
		typedef T (*F)(Context &);
		register const F expr(Read<F>(aContext));
		return (*expr)(aContext);
	}

	// nullary operator adapter
	template <typename T> T Nullary(Context &aContext, T (*aOp)())
	{
		return aOp();
	}

	// unary operator adapter
	template <typename T, typename A1> T Unary(Context &aContext, T (*aOp)(A1))
	{
		register const A1 arg1(Expression::Evaluate<A1>(aContext));
		return aOp(arg1);
	}

	// binary operator adapter
	template <typename T, typename A1, typename A2> T Binary(Context &aContext, T (*aOp)(A1, A2))
	{
		register const A1 arg1(Expression::Evaluate<A1>(aContext));
		register const A2 arg2(Expression::Evaluate<A2>(aContext));
		return aOp(arg1, arg2);
	};

	// ternary operator adapter
	template <typename T, typename A1, typename A2, typename A3> T Ternary(Context &aContext, T (*aOp)(A1, A2, A3))
	{
		register const A1 arg1(Expression::Evaluate<A1>(aContext));
		register const A2 arg2(Expression::Evaluate<A2>(aContext));
		register const A3 arg3(Expression::Evaluate<A3>(aContext));
		return aOp(arg1, arg2, arg3);
	};

	// componentwise nullary operator adapter
	template <typename T> T ComponentNullary(Context &aContext, float (*aOp)());

	// specialization for scalar type
	template <> inline float ComponentNullary(Context &aContext, float (*aOp)())
	{
		return Nullary<float>(aContext, aOp);
	};

	// componentwise unary operator adapter
	template <typename T> T ComponentUnary(Context &aContext, float (*aOp)(float));

	// specialization for scalar type
	template <> inline float ComponentUnary(Context &aContext, float (*aOp)(float))
	{
		return Unary<float, float>(aContext, aOp);
	}

	// componentwise binary operator adapter
	template <typename T> T ComponentBinary(Context &aContext, float (*aOp)(float, float));

	// specialization for scalar type
	template <> inline float ComponentBinary(Context &aContext, float (*aOp)(float, float))
	{
		return Binary<float, float, float>(aContext, aOp);
	}

	// componentwise ternary operator adapter
	template <typename T> T ComponentTernary(Context &aContext, float (*aOp)(float, float, float));

	// specialization for scalar type
	template <> inline float ComponentTernary(Context &aContext, float (*aOp)(float, float, float))
	{
		return Ternary<float, float, float, float>(aContext, aOp);
	}

	// construction expression
	template <typename T, typename A1> T Construct(Context &aContext)
	{
		register const A1 arg1(Evaluate<A1>(aContext));
		return T(arg1);
	}
	template <typename T, typename A1, typename A2> T Construct(Context &aContext)
	{
		register const A1 arg1(Evaluate<A1>(aContext));
		register const A2 arg2(Evaluate<A2>(aContext));
		return T(arg1, arg2);
	}
	template <typename T, typename A1, typename A2, typename A3> T Construct(Context &aContext)
	{
		register const A1 arg1(Evaluate<A1>(aContext));
		register const A2 arg2(Evaluate<A2>(aContext));
		register const A3 arg3(Evaluate<A3>(aContext));
		return T(arg1, arg2, arg3);
	}
	template <typename T, typename A1, typename A2, typename A3, typename A4> T Construct(Context &aContext)
	{
		register const A1 arg1(Evaluate<A1>(aContext));
		register const A2 arg2(Evaluate<A2>(aContext));
		register const A3 arg3(Evaluate<A3>(aContext));
		register const A4 arg4(Evaluate<A4>(aContext));
		return T(arg1, arg2, arg3, arg4);
	}
}

#include "ExpressionSIMD.h"
