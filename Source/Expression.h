#pragma once

namespace Expression
{
	// evaluation context
	struct Context
	{
		const unsigned int *mStream;
	};

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
	template <typename T, typename C> void Append(std::vector<unsigned int> &aBuffer, T (*aFunc)(C))
	{
		New<T (*)(C)>(aBuffer, aFunc);
	}
	template <typename T, typename C, typename D> void Append(std::vector<unsigned int> &aBuffer, T (*aFunc)(C), const D &aData)
	{
		New<T (*)(C)>(aBuffer, aFunc);
		New<D>(aBuffer, aData);
	}

	// read a value from an expression stream
	template <typename T> const T Read(Context &aContext)
	{
		const T &value = *reinterpret_cast<const T *>(aContext.mStream);
		aContext.mStream += sizeof(T)/sizeof(unsigned int);
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
			return Op();
		}
	};

	// unary operator adapter
	template <typename T, typename A1> struct Unary
	{
		template <typename OR, typename O1, OR Op(O1)> static const T Evaluate(Context &aContext)
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
			A1 arg1(Expression::Evaluate<A1>(aContext));
			A2 arg2(Expression::Evaluate<A2>(aContext));
			return Op(arg1, arg2);
		}
	};

	// componentwise nullary operator adapter
	// requres that T support operator[]
	template <typename T> struct ComponentNullary
	{
		template <typename OR, OR Op()> static const T Evaluate(Context &aContext)
		{
			T value = T();
			for (int i = 0; i < sizeof(T)/sizeof(float); ++i)
				value[i] = Op();
			return value;
		}
	};

	// componentwise unary operator adapter
	// requres that T support operator[]
	template <typename T> struct ComponentUnary
	{
		template <typename OR, typename O1, OR Op(O1)> static const T Evaluate(Context &aContext)
		{
			T arg1(Expression::Evaluate<T>(aContext));
			T value = T();
			for (int i = 0; i < sizeof(T)/sizeof(float); ++i)
				value[i] = Op(arg1[i]);
			return value;
		}
	};

	// componentwise binary operator adapter
	// requres that T support operator[]
	template <typename T> struct ComponentBinary
	{
		template <typename OR, typename O1, typename O2, OR Op(O1, O2)> static const T Evaluate(Context &aContext)
		{
			T arg1(Expression::Evaluate<T>(aContext));
			T arg2(Expression::Evaluate<T>(aContext));
			T value = T();
			for (int i = 0; i < sizeof(T)/sizeof(float); ++i)
				value[i] = Op(arg1[i], arg2[i]);
			return value;
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
