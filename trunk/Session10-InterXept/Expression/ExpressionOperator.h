#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"

namespace Expression
{
	//
	// ARITMETIC OPERATORS
	//

	// add
	template <typename T> T Add(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 + arg2;
	}

	// subtract
	template <typename T> T Sub(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 - arg2;
	}

	// multiply
	template <typename T> T Mul(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 * arg2;
	}

	// divide
	template <typename T> T Div(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 / arg2;
	}

	// reverse divide
	template <typename T> T DivR(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg2 / arg1;
	}

	// negate
	template <typename T> T Neg(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		return -arg1;
	}

	// reciprocal
	inline float Rcp(float v) { return 1.0f / v; };
	template <typename T> T Rcp(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Rcp>(aContext);
	}

	// increment
	inline float Inc(float v) { return v + 1.0f; };
	template <typename T> T Inc(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Inc>(aContext);
	}

	// decrement
	inline float Dec(float v) { return v - 1.0f; };
	template <typename T> T Dec(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Dec>(aContext);
	}


	//
	// TRIGONOMETRIC FUNCTIONS
	//

	// sine
	template <typename T> T Sin(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, sinf>(aContext);
	}

	// cosine
	template <typename T> T Cos(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, cosf>(aContext);
	}

	// tangent
	template <typename T> T Tan(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, tanf>(aContext);
	}

	// arcsine
	template <typename T> T Asin(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, asinf>(aContext);
	}

	// arccosine
	template <typename T> T Acos(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, acosf>(aContext);
	}

	// arctangent (1 argument)
	template <typename T> T Atan(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, atanf>(aContext);
	}

	// arctangent (2 arguments)
	template <typename T> T Atan2(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, atan2f>(aContext);
	}


	//
	// HYPERBOLIC FUNCTIONS
	//

	// hyperbolic sine
	template <typename T> T Sinh(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, sinhf>(aContext);
	}

	// hyperbolic cosine
	template <typename T> T Cosh(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, coshf>(aContext);
	}

	// hyperbolic tangent
	template <typename T> T Tanh(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, tanhf>(aContext);
	}


	//
	// EXPONENTIAL FUNCTIONS
	//

	// power
	template <typename T> T Pow(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, powf>(aContext);
	}

	// natural exponent
	template <typename T> T Exp(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, expf>(aContext);
	}

	// natural log
	template <typename T> T Log(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, logf>(aContext);
	}

	// square root
	template <typename T> T Sqrt(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, sqrtf>(aContext);
	}

	// reciprocal square root
	template <typename T> T InvSqrt(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, ::InvSqrt>(aContext);
	}


	//
	// COMMON FUNCTIONS
	//
	
	// absolute value
	template <typename T> T Abs(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, fabsf>(aContext);
	}

	// sign
	inline float Sign(float v) { return (v == 0) ? (0.0f) : ((v > 0) ? (1.0f) : (-1.0f)); }
	template <typename T> T Sign(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Sign>(aContext);
	}

	// floor
	template <typename T> T Floor(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, floorf>(aContext);
	}

	// ceiling
	template <typename T> T Ceil(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, ceilf>(aContext);
	}

	// fraction
	inline float Frac(float v) { return v - xs_FloorToInt(v); }
	template <typename T> T Frac(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Frac>(aContext);
	}

	// modulo
	template <typename T> T Mod(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, fmodf>(aContext);
	}

	// minimum
	template <typename T> T Min(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<const float &, const float &, const float &, std::min<float> >(aContext);
	}

	// maximum
	template <typename T> T Max(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<const float &, const float &, const float &, std::max<float> >(aContext);
	}

	// clamp
	template <typename T> T Clamp(Context &aContext)
	{
		return ComponentTernary<T, Schema<T>::COUNT>::Evaluate<const float, const float, const float, const float, ::Clamp<float> >(aContext);
	}

	// linear interpolate
	template <typename T> T Lerp(Context &aContext)
	{
		return Ternary<T, T, T, float>::Evaluate<const T, const T, const T, float, ::Lerp<T> >(aContext);
	}

	// step
	inline float Step(float e, float v) { return v < e ? 0.0f : 1.0f; }
	template <typename T> T Step(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, Step >(aContext);
	}

	// smooth step
	inline float SmoothStep(float e0, float e1, float v)
	{
		if (v <= e0) return 0.0f;
		if (v >= e1) return 1.0f;
		float t = (v - e0) / (e1 - e0);
		return t * t * (3 - 2 * t);
	}
	template <typename T> T SmoothStep(Context &aContext)
	{
		return ComponentTernary<T, Schema<T>::COUNT>::Evaluate<float, float, float, float, SmoothStep >(aContext);
	}
}

//
// UNARY EXPRESSION
// return the result of an expression taking one parameter
//
template <typename T, typename A, typename C> void ConfigureUnary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for unary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A>(arg1, buffer, names, defaults);
}


//
// BINARY EXPRESSION
// return the result of an expression taking two parameters
//
template <typename T, typename A1, typename A2, typename C> void ConfigureBinary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no first argument for binary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		assert(!"no second argument for binary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A1>(arg1, buffer, names, defaults);

	// append second argument
	ConfigureExpression<A2>(arg2, buffer, names, defaults);
}


//
// TERNARY EXPRESSION
// return the result of an expression taking three parameters
//
template <typename T, typename A1, typename A2, typename A3, typename C> void ConfigureTernary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no first argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		assert(!"no second argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg3 = arg2->NextSiblingElement();
	if (!arg3)
	{
		// no third argument: treat element as a literal (HACK)
		assert(!"no third argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A1>(arg1, buffer, names, defaults);

	// append second argument
	ConfigureExpression<A2>(arg2, buffer, names, defaults);

	// append third argument
	ConfigureExpression<A3>(arg3, buffer, names, defaults);
}


//
// VARIADIC EXPRESSION
// binary-to-variadic adapter
//

// configure typed variadic
template <typename T, typename A, typename C> void ConfigureVariadic(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		DebugPrint("no first argument for variadic operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: convert type of first argument (HACK)
		DebugPrint("no second argument for variadic operator: performing type conversion");
		Expression::Convert<T, A>::Append(buffer);
		ConfigureExpression<A>(arg1, buffer, names, defaults);
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
		ConfigureExpression<A>(arg1, buffer, names, defaults);
	}
	while (arg2);
}

#include "ExpressionOperatorSIMD.h"
