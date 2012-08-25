#pragma once

#include "Expression.h"

namespace Expression
{
	//
	// RELATIONAL OPERATORS
	//
	
	// greater than
	template <typename T> bool Greater(Context &aContext);

	// greater than or equal to
	template <typename T> bool GreaterEqual(Context &aContext);

	// less than
	template <typename T> bool Less(Context &aContext);

	// less than or equal to
	template <typename T> bool LessEqual(Context &aContext);

	// equal
	template <typename T> bool Equal(Context &aContext);

	// not equal
	template <typename T> bool NotEqual(Context &aContext);
}
