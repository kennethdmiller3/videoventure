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
	template <typename T> const T Read(Context &aContext)
	{
		const T &value = *reinterpret_cast<const T *>(aContext.mStream);
		aContext.mStream += (sizeof(T) + sizeof(unsigned int) - 1) / sizeof(unsigned int);
		return value;
	}

	// evaluate an expression stream
	template <typename T> const T Evaluate(Context &aContext)
	{
		typedef T (*F)(Context &);
		F expr(Read<F>(aContext));
		return (*expr)(aContext);
	}

	// constant expression
	template <typename T> const T Constant(Context &aContext)
	{
		return Read<T>(aContext);
	}

	// nullary operator adapter
	template <typename T> struct Nullary
	{
		template <typename OR, OR Op()> static T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR> static T Evaluate(Context &aContext, OR (*Op)())
		{
			return Op();
		}
	};

	// unary operator adapter
	template <typename T, typename A1> struct Unary
	{
		template <typename OR, typename O1, OR Op(O1)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1, OR Op(O1)> static const T Evaluate(Context &aContext, OR (*Op)(O1))
		{
			A1 arg1(Expression::Evaluate<A1>(aContext));
			return Op(arg1);
		}
	};

	// binary operator adapter
	template <typename T, typename A1, typename A2> struct Binary
	{
		template <typename OR, typename O1, typename O2, OR Op(O1, O2)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1, typename O2> static const T Evaluate(Context &aContext, OR (*Op)(O1, O2))
		{
			A1 arg1(Expression::Evaluate<A1>(aContext));
			A2 arg2(Expression::Evaluate<A2>(aContext));
			return Op(arg1, arg2);
		}
	};

	// ternary operator adapter
	template <typename T, typename A1, typename A2, typename A3> struct Ternary
	{
		template <typename OR, typename O1, typename O2, typename O3, OR Op(O1, O2, O3)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1, typename O2, typename O3> static const T Evaluate(Context &aContext, OR (*Op)(O1, O2, O3))
		{
			A1 arg1(Expression::Evaluate<A1>(aContext));
			A2 arg2(Expression::Evaluate<A2>(aContext));
			A3 arg3(Expression::Evaluate<A3>(aContext));
			return Op(arg1, arg2, arg3);
		}
	};

	// componentwise nullary operator adapter
	template <typename T, int W> struct ComponentNullary
	{
		// requres that T support operator[]
		template <typename OR, OR Op()> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR> static const T Evaluate(Context &aContext, OR (*Op)())
		{
			T value = T();
			for (int i = 0; i < W; ++i)
				value[i] = Op();
			return value;
		}
	};
	template <typename T> struct ComponentNullary<T, 1>
	{
		// specialization for scalar type
		template <typename OR, OR Op()> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR> static const T Evaluate(Context &aContext, OR (*Op)())
		{
			return Op();
		}
	};

	// componentwise unary operator adapter
	template <typename T, int W> struct ComponentUnary
	{
		// requres that T support operator[]
		template <typename OR, typename O1, OR Op(O1)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1> static const T Evaluate(Context &aContext, OR (*Op)(O1))
		{
			T arg1(Expression::Evaluate<T>(aContext));
			T value = T();
			for (int i = 0; i < W; ++i)
				value[i] = Op(arg1[i]);
			return value;
		}
	};
	template <typename T> struct ComponentUnary<T, 1>
	{
		// specialization for scalar type
		template <typename OR, typename O1, OR Op(O1)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1> static const T Evaluate(Context &aContext, OR (*Op)(O1))
		{
			T arg1(Expression::Evaluate<T>(aContext));
			return Op(arg1);
		}
	};

	// componentwise binary operator adapter
	template <typename T, int W> struct ComponentBinary
	{
		// requres that T support operator[]
		template <typename OR, typename O1, typename O2, OR Op(O1, O2)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1, typename O2> static const T Evaluate(Context &aContext, OR (*Op)(O1, O2))
		{
			T arg1(Expression::Evaluate<T>(aContext));
			T arg2(Expression::Evaluate<T>(aContext));
			T value = T();
			for (int i = 0; i < W; ++i)
				value[i] = Op(arg1[i], arg2[i]);
			return value;
		}
	};
	template <typename T> struct ComponentBinary<T, 1>
	{
		// specialization for scalar type
		template <typename OR, typename O1, typename O2, OR Op(O1, O2)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1, typename O2> static const T Evaluate(Context &aContext, OR (*Op)(O1, O2))
		{
			T arg1(Expression::Evaluate<T>(aContext));
			T arg2(Expression::Evaluate<T>(aContext));
			return Op(arg1, arg2);
		}
	};

	// componentwise ternary operator adapter
	template <typename T, int W> struct ComponentTernary
	{
		// requres that T support operator[]
		template <typename OR, typename O1, typename O2, typename O3, OR Op(O1, O2, O3)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1, typename O2, typename O3> static const T Evaluate(Context &aContext, OR (*Op)(O1, O2, O3))
		{
			T arg1(Expression::Evaluate<T>(aContext));
			T arg2(Expression::Evaluate<T>(aContext));
			T arg3(Expression::Evaluate<T>(aContext));
			T value = T();
			for (int i = 0; i < W; ++i)
				value[i] = Op(arg1[i], arg2[i], arg3[i]);
			return value;
		}
	};
	template <typename T> struct ComponentTernary<T, 1>
	{
		// specialization for scalar type
		template <typename OR, typename O1, typename O2, typename O3, OR Op(O1, O2, O3)> static const T Evaluate(Context &aContext)
		{
			return Evaluate(aContext, Op);
		}
		template <typename OR, typename O1, typename O2, typename O3> static const T Evaluate(Context &aContext, OR (*Op)(O1, O2, O3))
		{
			T arg1(Expression::Evaluate<T>(aContext));
			T arg2(Expression::Evaluate<T>(aContext));
			T arg3(Expression::Evaluate<T>(aContext));
			return Op(arg1, arg2, arg3);
		}
	};


	// construction expression
	template <typename T, typename A1> const T Construct(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		return T(arg1);
	}
	template <typename T, typename A1, typename A2> const T Construct(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		return T(arg1, arg2);
	}
	template <typename T, typename A1, typename A2, typename A3> const T Construct(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		A3 arg3(Evaluate<A3>(aContext));
		return T(arg1, arg2, arg3);
	}
	template <typename T, typename A1, typename A2, typename A3, typename A4> const T Construct(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		A3 arg3(Evaluate<A3>(aContext));
		A4 arg4(Evaluate<A4>(aContext));
		return T(arg1, arg2, arg3, arg4);
	}
}

#include "ExpressionSIMD.h"
