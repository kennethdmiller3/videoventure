#pragma once

namespace Expression
{
	// evaluation context
	struct Context
	{
		const unsigned int *mStart;
		const unsigned int *mStream;
	};

	// evaluate an expression stream
	template <typename T> const T Evaluate(Context &aContext)
	{
		typedef T (*F)(Context &);
		F expr = *reinterpret_cast<const F *>(aContext.mStream);
		aContext.mStream += sizeof(F)/sizeof(unsigned int);
		return (*expr)(aContext);
	}

	// constant expression
	template <typename T> const T Constant(Context &aContext)
	{
		const T &value = *reinterpret_cast<const T *>(aContext.mStream);
		aContext.mStream += sizeof(T)/sizeof(unsigned int);
		return value;
	}

	// nullary operator adapter
	template <typename R, R Op()> const R Nullary(Context &aContext)
	{
		return Op();
	}

	// unary operator adapter
	template <typename R, typename A, R Op(A)> const R Unary(Context &aContext)
	{
		A arg(Evaluate<A>(aContext));
		return Op(arg);
	}
	template <typename R, typename A, R Op(A &)> const R UnaryRef(Context &aContext)
	{
		A arg(Evaluate<A>(aContext));
		return Op(arg);
	}

	// binary operator adapter
	template <typename R, typename A1, typename A2, R Op(A1, A2)> const R Binary(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		return Op(arg1, arg2);
	}
	template <typename R, typename A1, typename A2, R Op(A1 &, A2)> const R BinaryRef1(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		return Op(arg1, arg2);
	}
	template <typename R, typename A1, typename A2, R Op(A1, A2 &)> const R BinaryRef2(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		return Op(arg1, arg2);
	}
	template <typename R, typename A1, typename A2, R Op(A1 &, A2 &)> const R BinaryRef1Ref2(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		return Op(arg1, arg2);
	}

	// allocate data from a buffer
	inline void *Alloc(std::vector<unsigned int> &buffer, size_t size)
	{
		size /= sizeof(unsigned int);
		buffer.resize(buffer.size() + size);
		return &buffer[buffer.size() - size];
	}

	// allocate object from a buffer
	template <typename T> T *New(std::vector<unsigned int> &buffer)
	{
		return new(Alloc(buffer, sizeof(T))) T();
	}
	template <typename T, typename A1> T *New(std::vector<unsigned int> &buffer, A1 arg1)
	{
		return new(Alloc(buffer, sizeof(T))) T(arg1);
	}
	template <typename T, typename A1, typename A2> T *New(std::vector<unsigned int> &buffer, A1 arg1, A2 arg2)
	{
		return new(Alloc(buffer, sizeof(T))) T(arg1, arg2);
	}
	template <typename T, typename A1, typename A2, typename A3> T *New(std::vector<unsigned int> &buffer, A1 arg1, A2 arg2, A3 arg3)
	{
		return new(Alloc(buffer, sizeof(T))) T(arg1, arg2, arg3);
	}
	template <typename T, typename A1, typename A2, typename A3, typename A4> T *New(std::vector<unsigned int> &buffer, A1 arg1, A2 arg2, A3 arg3, A4 arg4)
	{
		return new(Alloc(buffer, sizeof(T))) T(arg1, arg2, arg3, arg4);
	}

	// append an expression to a buffer
	template <typename T, typename A> void Append(std::vector<unsigned int> &aBuffer, T (*aFunc)(A))
	{
		New<T (*)(A)>(aBuffer, aFunc);
	}
	template <typename T, typename A, typename D> void Append(std::vector<unsigned int> &aBuffer, T (*aFunc)(A), D aData)
	{
		New<T (*)(A)>(aBuffer, aFunc);
		New<D>(aBuffer, aData);
	}


	// construction expression
	template <typename R, typename A> const R Construct(Context &aContext)
	{
		A arg(Evaluate<A>(aContext));
		return R(arg);
	}
	template <typename R, typename A1, typename A2> const R Construct(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		return R(arg1, arg2);
	}
	template <typename R, typename A1, typename A2, typename A3> const R Construct(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		A3 arg3(Evaluate<A3>(aContext));
		return R(arg1, arg2, arg3);
	}
	template <typename R, typename A1, typename A2, typename A3, typename A4> const R Construct(Context &aContext)
	{
		A1 arg1(Evaluate<A1>(aContext));
		A2 arg2(Evaluate<A2>(aContext));
		A3 arg3(Evaluate<A3>(aContext));
		A4 arg4(Evaluate<A4>(aContext));
		return R(arg1, arg2, arg3, arg4);
	}
}
