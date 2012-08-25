#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionConvert.h"

namespace Expression
{
	//
	// ARITMETIC OPERATORS
	//

	// add
	template <typename T> T Add(Context &aContext);

	// subtract
	template <typename T> T Sub(Context &aContext);

	// multiply
	template <typename T> T Mul(Context &aContext);

	// divide
	template <typename T> T Div(Context &aContext);

	// reverse divide
	template <typename T> T DivR(Context &aContext);

	// negate
	template <typename T> T Neg(Context &aContext);

	// reciprocal
	template <typename T> T Rcp(Context &aContext);

	// increment
	template <typename T> T Inc(Context &aContext);

	// decrement
	template <typename T> T Dec(Context &aContext);


	//
	// TRIGONOMETRIC FUNCTIONS
	//

	// sine
	template <typename T> T Sin(Context &aContext);

	// cosine
	template <typename T> T Cos(Context &aContext);

	// tangent
	template <typename T> T Tan(Context &aContext);

	// arcsine
	template <typename T> T Asin(Context &aContext);

	// arccosine
	template <typename T> T Acos(Context &aContext);

	// arctangent (1 argument)
	template <typename T> T Atan(Context &aContext);

	// arctangent (2 arguments)
	template <typename T> T Atan2(Context &aContext);


	//
	// HYPERBOLIC FUNCTIONS
	//

	// hyperbolic sine
	template <typename T> T Sinh(Context &aContext);

	// hyperbolic cosine
	template <typename T> T Cosh(Context &aContext);

	// hyperbolic tangent
	template <typename T> T Tanh(Context &aContext);


	//
	// EXPONENTIAL FUNCTIONS
	//

	// power
	template <typename T> T Pow(Context &aContext);

	// natural exponent
	template <typename T> T Exp(Context &aContext);

	// natural log
	template <typename T> T Log(Context &aContext);

	// square root
	template <typename T> T Sqrt(Context &aContext);

	// reciprocal square root
	template <typename T> T InvSqrt(Context &aContext);


	//
	// COMMON FUNCTIONS
	//
	
	// absolute value
	template <typename T> T Abs(Context &aContext);

	// sign
	template <typename T> T Sign(Context &aContext);

	// floor
	template <typename T> T Floor(Context &aContext);

	// ceiling
	template <typename T> T Ceil(Context &aContext);

	// fraction
	template <typename T> T Frac(Context &aContext);

	// modulo
	template <typename T> T Mod(Context &aContext);

	// minimum
	template <typename T> T Min(Context &aContext);

	// maximum
	template <typename T> T Max(Context &aContext);

	// clamp
	template <typename T> T Clamp(Context &aContext);

	// linear interpolate
	template <typename T> T Lerp(Context &aContext);

	// step
	template <typename T> T Step(Context &aContext);

	// smooth step
	template <typename T> T SmoothStep(Context &aContext);
}


//
// UNARY EXPRESSION
// return the result of an expression taking one parameter
//
template <typename T, typename A, typename C> void ConfigureUnary(T (expr)(C), const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		DebugPrint("no argument for unary operator %s", element->Value());
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	Expression::Loader<A>::Configure(arg1, buffer, names, defaults);
}


//
// BINARY EXPRESSION
// return the result of an expression taking two parameters
//
template <typename T, typename A1, typename A2, typename C> void ConfigureBinary(T (expr)(C), const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		DebugPrint("no first argument for binary operator %s", element->Value());
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const tinyxml2::XMLElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		DebugPrint("no second argument for binary operator %s", element->Value());
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	Expression::Loader<A1>::Configure(arg1, buffer, names, defaults);

	// append second argument
	Expression::Loader<A2>::Configure(arg2, buffer, names, defaults);
}


//
// TERNARY EXPRESSION
// return the result of an expression taking three parameters
//
template <typename T, typename A1, typename A2, typename A3, typename C> void ConfigureTernary(T (expr)(C), const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		DebugPrint("no first argument for ternary operator %s", element->Value());
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const tinyxml2::XMLElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		DebugPrint("no second argument for ternary operator %s", element->Value());
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const tinyxml2::XMLElement *arg3 = arg2->NextSiblingElement();
	if (!arg3)
	{
		// no third argument: treat element as a literal (HACK)
		DebugPrint("no third argument for ternary operator %s", element->Value());
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	Expression::Loader<A1>::Configure(arg1, buffer, names, defaults);

	// append second argument
	Expression::Loader<A2>::Configure(arg2, buffer, names, defaults);

	// append third argument
	Expression::Loader<A3>::Configure(arg3, buffer, names, defaults);
}


//
// VARIADIC EXPRESSION
// binary-to-variadic adapter
//

// configure typed variadic
template <typename T, typename A, typename C> void ConfigureVariadic(T (expr)(C), const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		DebugPrint("no first argument for variadic operator %s", element->Value());
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const tinyxml2::XMLElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: convert type of first argument (HACK)
		DebugPrint("no second argument for variadic operator %s", element->Value());
		Expression::Convert<T, A>::Append(buffer);
		Expression::Loader<A>::Configure(arg1, buffer, names, defaults);
		return;
	}

	// rewind
	arg2 = arg1;
	do
	{
		// get next argument
		arg1 = arg2;
		arg2 = arg2->NextSiblingElement();

		// if there is a second argument...
		if (arg2)
		{
			// append the operator
			Expression::Append(buffer, expr);
		}

		// append first argument
		Expression::Loader<A>::Configure(arg1, buffer, names, defaults);
	}
	while (arg2);
}

#include "ExpressionOperatorSIMD.h"
