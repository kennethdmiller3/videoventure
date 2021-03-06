#include "StdAfx.h"

#include "ExpressionOperator.h"
#include "ExpressionConfigure.h"

namespace Expression
{
	//
	// ARITMETIC OPERATORS
	//

	// add
	template <typename T> T Add(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		const T arg2(Evaluate<T>(aContext));
		return arg1 + arg2;
	}

	// subtract
	template <typename T> T Sub(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		const T arg2(Evaluate<T>(aContext));
		return arg1 - arg2;
	}

	// multiply
	template <typename T> T Mul(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		const T arg2(Evaluate<T>(aContext));
		return arg1 * arg2;
	}

	// divide
	template <typename T> T Div(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		const T arg2(Evaluate<T>(aContext));
		return arg1 / arg2;
	}

	// reverse divide
	template <typename T> T DivR(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		const T arg2(Evaluate<T>(aContext));
		return arg2 / arg1;
	}

	// negate
	template <typename T> T Neg(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		return -arg1;
	}

	// reciprocal
	template <typename T> T Rcp(Context &aContext)
	{
		return Unary<T, T>(aContext, ::Rcp);
	}

	// increment
	template <typename T> T Inc(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		return arg1 + ::Extend<T>(1);
	}

	// decrement
	template <typename T> T Dec(Context &aContext)
	{
		const T arg1(Evaluate<T>(aContext));
		return arg1 - ::Extend<T>(1);
	}


	//
	// TRIGONOMETRIC FUNCTIONS
	//

	// sine
	template <typename T> T Sin(Context &aContext)
	{
		return ComponentUnary<T>(aContext, sinf);
	}

	// cosine
	template <typename T> T Cos(Context &aContext)
	{
		return ComponentUnary<T>(aContext, cosf);
	}

	// tangent
	template <typename T> T Tan(Context &aContext)
	{
		return ComponentUnary<T>(aContext, tanf);
	}

	// arcsine
	template <typename T> T Asin(Context &aContext)
	{
		return ComponentUnary<T>(aContext, asinf);
	}

	// arccosine
	template <typename T> T Acos(Context &aContext)
	{
		return ComponentUnary<T>(aContext, acosf);
	}

	// arctangent (1 argument)
	template <typename T> T Atan(Context &aContext)
	{
		return ComponentUnary<T>(aContext, atanf);
	}

	// arctangent (2 arguments)
	template <typename T> T Atan2(Context &aContext)
	{
		return ComponentBinary<T>(aContext, atan2f);
	}


	//
	// HYPERBOLIC FUNCTIONS
	//

	// hyperbolic sine
	template <typename T> T Sinh(Context &aContext)
	{
		return ComponentUnary<T>(aContext, sinhf);
	}

	// hyperbolic cosine
	template <typename T> T Cosh(Context &aContext)
	{
		return ComponentUnary<T>(aContext, coshf);
	}

	// hyperbolic tangent
	template <typename T> T Tanh(Context &aContext)
	{
		return ComponentUnary<T>(aContext, tanhf);
	}


	//
	// EXPONENTIAL FUNCTIONS
	//

	// power
	template <typename T> T Pow(Context &aContext)
	{
		return ComponentBinary<T>(aContext, powf);
	}

	// natural exponent
	template <typename T> T Exp(Context &aContext)
	{
		return ComponentUnary<T>(aContext, expf);
	}

	// natural log
	template <typename T> T Log(Context &aContext)
	{
		return ComponentUnary<T>(aContext, logf);
	}

	// square root
	template <typename T> T Sqrt(Context &aContext)
	{
		return Unary<T>(aContext, ::Sqrt);
	}

	// reciprocal square root
	template <typename T> T InvSqrt(Context &aContext)
	{
		return Unary<T>(aContext, ::InvSqrt);
	}


	//
	// COMMON FUNCTIONS
	//
	
	// absolute value
	template <typename T> T Abs(Context &aContext)
	{
		return Unary<T>(aContext, ::Abs);
	}

	// sign
	template <typename T> T Sign(Context &aContext)
	{
		return ComponentUnary<T>(aContext, ::Sign);
	}

	// floor
	template <typename T> T Floor(Context &aContext)
	{
		return ComponentUnary<T>(aContext, floorf);
	}

	// ceiling
	template <typename T> T Ceil(Context &aContext)
	{
		return ComponentUnary<T>(aContext, ceilf);
	}

	// fraction
	template <typename T> T Frac(Context &aContext)
	{
		return ComponentUnary<T>(aContext, ::Frac);
	}

	// modulo
	template <typename T> T Mod(Context &aContext)
	{
		return ComponentBinary<T>(aContext, fmodf);
	}

	// minimum
	template <typename T> T Min(Context &aContext)
	{
		return Binary<T, T, T>(aContext, ::Min);
	}

	// maximum
	template <typename T> T Max(Context &aContext)
	{
		return Binary<T, T, T>(aContext, ::Max);
	}

	// clamp
	template <typename T> T Clamp(Context &aContext)
	{
		return Ternary<T, T, T, T>(aContext, ::Clamp);
	}

	// linear interpolate
	template <typename T> T Lerp(Context &aContext)
	{
		return Ternary<T, T, T, float>(aContext, ::Lerp);
	}

	// step
	template <typename T> T Step(Context &aContext)
	{
		return Binary<T, T, T>(aContext, ::Step);
	}

	// smooth step
	template <typename T> T SmoothStep(Context &aContext)
	{
		return Ternary<T, T, T, T>(aContext, ::SmoothStep);
	}
}

//
template<typename T> static void ConfigureAdd(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Add<T>, element, buffer, names, data);
}

static Expression::Loader<float> addfloat(0x3b391274 /* "add" */, ConfigureAdd<float>);
static Expression::Loader<__m128> addvector(0x3b391274 /* "add" */, ConfigureAdd<__m128>);

//
template<typename T> static void ConfigureSub(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Sub<T>, element, buffer, names, data);
}

static Expression::Loader<float> subfloat(0xdc4e3915 /* "sub" */, ConfigureSub<float>);
static Expression::Loader<__m128> subvector(0xdc4e3915 /* "sub" */, ConfigureSub<__m128>);

//
template<typename T> static void ConfigureMul(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Mul<T>, element, buffer, names, data);
}

static Expression::Loader<float> mulfloat(0xeb84ed81 /* "mul" */, ConfigureMul<float>);
static Expression::Loader<__m128> mulvector(0xeb84ed81 /* "mul" */, ConfigureMul<__m128>);

//
template<typename T> static void ConfigureDiv(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Div<T>, element, buffer, names, data);
}

static Expression::Loader<float> divfloat(0xe562ab48 /* "div" */, ConfigureDiv<float>);
static Expression::Loader<__m128> divvector(0xe562ab48 /* "div" */, ConfigureDiv<__m128>);

//
template<typename T> static void ConfigureNeg(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Neg<T>, element, buffer, names, data);
}

static Expression::Loader<float> negfloat(0x3899af41 /* "neg" */, ConfigureNeg<float>);
static Expression::Loader<__m128> negvector(0x3899af41 /* "neg" */, ConfigureNeg<__m128>);

//
template<typename T> static void ConfigureRcp(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Rcp<T>, element, buffer, names, data);
}

static Expression::Loader<float> rcpfloat(0x31037236 /* "rcp" */, ConfigureRcp<float>);
static Expression::Loader<__m128> rcpvector(0x31037236 /* "rcp" */, ConfigureRcp<__m128>);

//
template<typename T> static void ConfigureInc(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Inc<T>, element, buffer, names, data);
}

static Expression::Loader<float> incfloat(0xa8e99c47 /* "inc" */, ConfigureInc<float>);
static Expression::Loader<__m128> incvector(0xa8e99c47 /* "inc" */, ConfigureInc<__m128>);

//
template<typename T> static void ConfigureDec(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Dec<T>, element, buffer, names, data);
}

static Expression::Loader<float> decfloat(0xc25979d3 /* "dec" */, ConfigureDec<float>);
static Expression::Loader<__m128> decvector(0xc25979d3 /* "dec" */, ConfigureDec<__m128>);

//
template<typename T> static void ConfigureSin(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sin<T>, element, buffer, names, data);
}

static Expression::Loader<float> sinfloat(0xe0302a4d /* "sin" */, ConfigureSin<float>);
static Expression::Loader<__m128> sinvector(0xe0302a4d /* "sin" */, ConfigureSin<__m128>);

//
template<typename T> static void ConfigureCos(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Cos<T>, element, buffer, names, data);
}

static Expression::Loader<float> cosfloat(0xfb8de29c /* "cos" */, ConfigureCos<float>);
static Expression::Loader<__m128> cosvector(0xfb8de29c /* "cos" */, ConfigureCos<__m128>);

//
template<typename T> static void ConfigureTan(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Tan<T>, element, buffer, names, data);
}

static Expression::Loader<float> tanfloat(0x9cf73498 /* "tan" */, ConfigureTan<float>);
static Expression::Loader<__m128> tanvector(0x9cf73498 /* "tan" */, ConfigureTan<__m128>);

//
template<typename T> static void ConfigureAsin(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Asin<T>, element, buffer, names, data);
}

static Expression::Loader<float> asinfloat(0xfeae7ea6 /* "asin" */, ConfigureAsin<float>);
static Expression::Loader<__m128> asinvector(0xfeae7ea6 /* "asin" */, ConfigureAsin<__m128>);

//
template<typename T> static void ConfigureAcos(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Acos<T>, element, buffer, names, data);
}

static Expression::Loader<float> acosfloat(0x3c01df1f /* "acos" */, ConfigureAcos<float>);
static Expression::Loader<__m128> acosvector(0x3c01df1f /* "acos" */, ConfigureAcos<__m128>);

//
template<typename T> static void ConfigureAtan(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Atan<T>, element, buffer, names, data);
}

static Expression::Loader<float> atanfloat(0x0678cabf /* "atan" */, ConfigureAtan<float>);
static Expression::Loader<__m128> atanvector(0x0678cabf /* "atan" */, ConfigureAtan<__m128>);

//
template<typename T> static void ConfigureAtan2(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureBinary<T, T, T>(Expression::Atan2<T>, element, buffer, names, data);
}

static Expression::Loader<float> atan2float(0xbd26dbf7 /* "atan2" */, ConfigureAtan2<float>);
static Expression::Loader<__m128> atan2vector(0xbd26dbf7 /* "atan2" */, ConfigureAtan2<__m128>);

//
template<typename T> static void ConfigureSinh(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sinh<T>, element, buffer, names, data);
}

static Expression::Loader<float> sinhfloat(0x10d2583f /* "sinh" */, ConfigureSinh<float>);
static Expression::Loader<__m128> sinhvector(0x10d2583f /* "sinh" */, ConfigureSinh<__m128>);

//
template<typename T> static void ConfigureCosh(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Cosh<T>, element, buffer, names, data);
}

static Expression::Loader<float> coshfloat(0xf45c461c /* "cosh" */, ConfigureCosh<float>);
static Expression::Loader<__m128> coshvector(0xf45c461c /* "cosh" */, ConfigureCosh<__m128>);

//
template<typename T> static void ConfigureTanh(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Tanh<T>, element, buffer, names, data);
}

static Expression::Loader<float> tanhfloat(0x092855d0 /* "tanh" */, ConfigureTanh<float>);
static Expression::Loader<__m128> tanhvector(0x092855d0 /* "tanh" */, ConfigureTanh<__m128>);

//
template<typename T> static void ConfigurePow(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureBinary<T, T, T>(Expression::Pow<T>, element, buffer, names, data);
}

static Expression::Loader<float> powfloat(0x58336ad5 /* "pow" */, ConfigurePow<float>);
static Expression::Loader<__m128> powvector(0x58336ad5 /* "pow" */, ConfigurePow<__m128>);

//
template<typename T> static void ConfigureExp(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Exp<T>, element, buffer, names, data);
}

static Expression::Loader<float> expfloat(0x72a68728 /* "exp" */, ConfigureExp<float>);
static Expression::Loader<__m128> expvector(0x72a68728 /* "exp" */, ConfigureExp<__m128>);

//
template<typename T> static void ConfigureLog(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Log<T>, element, buffer, names, data);
}

static Expression::Loader<float> logfloat(0x3f515151 /* "log" */, ConfigureLog<float>);
static Expression::Loader<__m128> logvector(0x3f515151 /* "log" */, ConfigureLog<__m128>);

//
template<typename T> static void ConfigureSqrt(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sqrt<T>, element, buffer, names, data);
}

static Expression::Loader<float> sqrtfloat(0x7dee3bcf /* "sqrt" */, ConfigureSqrt<float>);
static Expression::Loader<__m128> sqrtvector(0x7dee3bcf /* "sqrt" */, ConfigureSqrt<__m128>);

//
template<typename T> static void ConfigureInvSqrt(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::InvSqrt<T>, element, buffer, names, data);
}

static Expression::Loader<float> invsqrtfloat(0x0a6a5946 /* "invsqrt" */, ConfigureInvSqrt<float>);
static Expression::Loader<__m128> invsqrtvector(0x0a6a5946 /* "invsqrt" */, ConfigureInvSqrt<__m128>);

//
template<typename T> static void ConfigureAbs(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Abs<T>, element, buffer, names, data);
}

static Expression::Loader<float> absfloat(0x2a48023b /* "abs" */, ConfigureAbs<float>);
static Expression::Loader<__m128> absvector(0x2a48023b /* "abs" */, ConfigureAbs<__m128>);

//
template<typename T> static void ConfigureSign(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sign<T>, element, buffer, names, data);
}

static Expression::Loader<float> signfloat(0x0cbc8ba4 /* "sign" */, ConfigureSign<float>);
static Expression::Loader<__m128> signvector(0x0cbc8ba4 /* "sign" */, ConfigureSign<__m128>);

//
template<typename T> static void ConfigureFloor(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Floor<T>, element, buffer, names, data);
}

static Expression::Loader<float> floorfloat(0xb8e70c1d /* "floor" */, ConfigureFloor<float>);
static Expression::Loader<__m128> floorvector(0xb8e70c1d /* "floor" */, ConfigureFloor<__m128>);

//
template<typename T> static void ConfigureCeil(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Ceil<T>, element, buffer, names, data);
}

static Expression::Loader<float> ceilfloat(0x62e4e208 /* "ceil" */, ConfigureCeil<float>);
static Expression::Loader<__m128> ceilvector(0x62e4e208 /* "ceil" */, ConfigureCeil<__m128>);

//
template<typename T> static void ConfigureFrac(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Frac<T>, element, buffer, names, data);
}

static Expression::Loader<float> fracfloat(0x87aad829 /* "frac" */, ConfigureFrac<float>);
static Expression::Loader<__m128> fracvector(0x87aad829 /* "frac" */, ConfigureFrac<__m128>);

//
template<typename T> static void ConfigureMod(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureBinary<T, T, T>(Expression::Mod<T>, element, buffer, names, data);
}

static Expression::Loader<float> modfloat(0xdf9e7283 /* "mod" */, ConfigureMod<float>);
static Expression::Loader<__m128> modvector(0xdf9e7283 /* "mod" */, ConfigureMod<__m128>);

//
template<typename T> static void ConfigureMin(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Min<T>, element, buffer, names, data);
}

static Expression::Loader<float> minfloat(0xc98f4557 /* "min" */, ConfigureMin<float>);
static Expression::Loader<__m128> minvector(0xc98f4557 /* "min" */, ConfigureMin<__m128>);

//
template<typename T> static void ConfigureMax(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Max<T>, element, buffer, names, data);
}

static Expression::Loader<float> maxfloat(0xd7a2e319 /* "max" */, ConfigureMax<float>);
static Expression::Loader<__m128> maxvector(0xd7a2e319 /* "max" */, ConfigureMax<__m128>);

//
template<typename T> static void ConfigureClamp(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureTernary<T, T, T, T>(Expression::Clamp<T>, element, buffer, names, data);
}

static Expression::Loader<float> clampfloat(0xa82efcbc /* "clamp" */, ConfigureClamp<float>);
static Expression::Loader<__m128> clampvector(0xa82efcbc /* "clamp" */, ConfigureClamp<__m128>);

//
template<typename T> static void ConfigureLerp(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureTernary<T, T, T, float>(Expression::Lerp<T>, element, buffer, names, data);
}

static Expression::Loader<float> lerpfloat(0x1e691468 /* "lerp" */, ConfigureLerp<float>);
static Expression::Loader<__m128> lerpvector(0x1e691468 /* "lerp" */, ConfigureLerp<__m128>);

//
template<typename T> static void ConfigureStep(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureBinary<T, T, float>(Expression::Step<T>, element, buffer, names, data);
}

static Expression::Loader<float> stepfloat(0xc7441a0f /* "step" */, ConfigureStep<float>);
static Expression::Loader<__m128> stepvector(0xc7441a0f /* "step" */, ConfigureStep<__m128>);

//
template<typename T> static void ConfigureSmoothStep(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureTernary<T, T, T, float>(Expression::SmoothStep<T>, element, buffer, names, data);
}

static Expression::Loader<float> smoothstepfloat(0x95964e7d /* "smoothstep" */, ConfigureSmoothStep<float>);
static Expression::Loader<__m128> smoothstepvector(0x95964e7d /* "smoothstep" */, ConfigureSmoothStep<__m128>);
