#pragma once

#include "Expression.h"

namespace Expression
{
	//
	// LOGICAL OPERATORS
	//

	// logical and
	bool And(Context &aContext);

	// logical or
	bool Or(Context &aContext);

	// logical not
	bool Not(Context &aContext);

	// logical xor
	bool Xor(Context &aContext);
}
