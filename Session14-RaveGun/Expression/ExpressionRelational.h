#pragma once

#include "Expression.h"

namespace Expression
{
	//
	// RELATIONAL OPERATORS
	//
	
	// greater than
	template <typename T> bool Greater(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 > arg2;
	}

	// greater than or equal to
	template <typename T> bool GreaterEqual(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 >= arg2;
	}

	// less than
	template <typename T> bool Less(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 < arg2;
	}

	// less than or equal to
	template <typename T> bool LessEqual(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 <= arg2;
	}

	// equal
	template <typename T> bool Equal(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 == arg2;
	}

	// not equal
	template <typename T> bool NotEqual(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 != arg2;
	}
}
