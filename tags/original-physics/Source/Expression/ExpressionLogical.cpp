#include "StdAfx.h"

#include "ExpressionLogical.h"
#include "ExpressionConfigure.h"

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
		bool value = true;
		do
		{
			value = Evaluate<bool>(aContext);
		}
		while (value && aContext.mStream < end);

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
		bool value = false;
		do
		{
			value = Evaluate<bool>(aContext);
		}
		while (!value && aContext.mStream < end);

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

static void ConfigureShortCircuit(bool (*expr)(Expression::Context &), const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: return false (HACK)
		DebugPrint("no first argument for variadic operator %s", element->Value());
		Expression::Append(buffer, Expression::Constant<bool>, false);
		return;
	}

	const tinyxml2::XMLElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: pass first argument through (HACK)
		DebugPrint("no second argument for variadic operator %s", element->Value());
		Expression::Loader<bool>::Configure(arg1, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append all arguments
	buffer.push_back(0);
	int start = buffer.size();
	do
	{
		Expression::Loader<bool>::Configure(arg1, buffer, names, defaults);
		arg1 = arg1->NextSiblingElement();
	}
	while (arg1);
	buffer[start - 1] = buffer.size() - start;
}

static void ConfigureAnd(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureShortCircuit(Expression::And, element, buffer, names, defaults);
}
static Expression::Loader<bool> andbool(0x0f29c2a6 /* "and" */, ConfigureAnd);

static void ConfigureOr(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureShortCircuit(Expression::Or, element, buffer, names, defaults);
}
static Expression::Loader<bool> orbool(0x5d342984 /* "or" */, ConfigureOr);

static void ConfigureNot(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureUnary<bool, bool>(Expression::Not, element, buffer, names, defaults);
}
static Expression::Loader<bool> notbool(0x29b19c8a /* "not" */, ConfigureNot);

static void ConfigureXor(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureVariadic<bool, bool>(Expression::Xor, element, buffer, names, defaults);
}
static Expression::Loader<bool> xorbool(0xcc6bdb7e /* "xor" */, ConfigureXor);
