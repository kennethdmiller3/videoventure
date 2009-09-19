#include "StdAfx.h"

#include "ExpressionLogical.h"

namespace Expression
{
	//
	// LOGICAL OPERATORS
	//

	// logical and
	bool And(Context &aContext)
	{
		// data size
		unsigned int size = Read<unsigned int>(aContext);

		// end of data
		const unsigned int *end = aContext.mStream + size;

		// get value
		bool value = Evaluate<bool>(aContext) && Evaluate<bool>(aContext);

		// advance stream
		aContext.mStream = end;

		// return value
		return value;
	}

	// logical or
	bool Or(Context &aContext)
	{
		// data size
		unsigned int size = Read<unsigned int>(aContext);

		// end of data
		const unsigned int *end = aContext.mStream + size;

		// get value
		bool value = Evaluate<bool>(aContext) || Evaluate<bool>(aContext);

		// advance stream
		aContext.mStream = end;

		// return value
		return value;
	}

	// logical not
	bool Not(Context &aContext)
	{
		return !Evaluate<bool>(aContext);
	}

	// logical xor
	bool Xor(Context &aContext)
	{
		return Evaluate<bool>(aContext) != Evaluate<bool>(aContext);
	}
}