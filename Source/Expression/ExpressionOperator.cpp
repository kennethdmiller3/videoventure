#include "StdAfx.h"

#include "ExpressionOperator.h"
#include "ExpressionConfigure.h"

//
template<typename T> static void ConfigureAdd(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Add<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> addfloat(0x3b391274 /* "add" */, ConfigureAdd<float>);
static ExpressionConfigure::Auto<__m128> addvector(0x3b391274 /* "add" */, ConfigureAdd<__m128>);

//
template<typename T> static void ConfigureSub(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Sub<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> subfloat(0xdc4e3915 /* "sub" */, ConfigureSub<float>);
static ExpressionConfigure::Auto<__m128> subvector(0xdc4e3915 /* "sub" */, ConfigureSub<__m128>);

//
template<typename T> static void ConfigureMul(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Mul<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> mulfloat(0xeb84ed81 /* "mul" */, ConfigureMul<float>);
static ExpressionConfigure::Auto<__m128> mulvector(0xeb84ed81 /* "mul" */, ConfigureMul<__m128>);

//
template<typename T> static void ConfigureDiv(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Div<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> divfloat(0xe562ab48 /* "div" */, ConfigureDiv<float>);
static ExpressionConfigure::Auto<__m128> divvector(0xe562ab48 /* "div" */, ConfigureDiv<__m128>);

//
template<typename T> static void ConfigureNeg(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Neg<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> negfloat(0x3899af41 /* "neg" */, ConfigureNeg<float>);
static ExpressionConfigure::Auto<__m128> negvector(0x3899af41 /* "neg" */, ConfigureNeg<__m128>);

//
template<typename T> static void ConfigureRcp(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Rcp<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> rcpfloat(0x31037236 /* "rcp" */, ConfigureRcp<float>);
static ExpressionConfigure::Auto<__m128> rcpvector(0x31037236 /* "rcp" */, ConfigureRcp<__m128>);

//
template<typename T> static void ConfigureInc(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Inc<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> incfloat(0xa8e99c47 /* "inc" */, ConfigureInc<float>);
static ExpressionConfigure::Auto<__m128> incvector(0xa8e99c47 /* "inc" */, ConfigureInc<__m128>);

//
template<typename T> static void ConfigureDec(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Dec<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> decfloat(0xc25979d3 /* "dec" */, ConfigureDec<float>);
static ExpressionConfigure::Auto<__m128> decvector(0xc25979d3 /* "dec" */, ConfigureDec<__m128>);

//
template<typename T> static void ConfigureSin(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sin<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> sinfloat(0xe0302a4d /* "sin" */, ConfigureSin<float>);
static ExpressionConfigure::Auto<__m128> sinvector(0xe0302a4d /* "sin" */, ConfigureSin<__m128>);

//
template<typename T> static void ConfigureCos(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Cos<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> cosfloat(0xfb8de29c /* "cos" */, ConfigureCos<float>);
static ExpressionConfigure::Auto<__m128> cosvector(0xfb8de29c /* "cos" */, ConfigureCos<__m128>);

//
template<typename T> static void ConfigureTan(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Tan<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> tanfloat(0x9cf73498 /* "tan" */, ConfigureTan<float>);
static ExpressionConfigure::Auto<__m128> tanvector(0x9cf73498 /* "tan" */, ConfigureTan<__m128>);

//
template<typename T> static void ConfigureAsin(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Asin<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> asinfloat(0xfeae7ea6 /* "asin" */, ConfigureAsin<float>);
static ExpressionConfigure::Auto<__m128> asinvector(0xfeae7ea6 /* "asin" */, ConfigureAsin<__m128>);

//
template<typename T> static void ConfigureAcos(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Acos<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> acosfloat(0x3c01df1f /* "acos" */, ConfigureAcos<float>);
static ExpressionConfigure::Auto<__m128> acosvector(0x3c01df1f /* "acos" */, ConfigureAcos<__m128>);

//
template<typename T> static void ConfigureAtan(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Atan<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> atanfloat(0x0678cabf /* "atan" */, ConfigureAtan<float>);
static ExpressionConfigure::Auto<__m128> atanvector(0x0678cabf /* "atan" */, ConfigureAtan<__m128>);

//
template<typename T> static void ConfigureAtan2(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureBinary<T, T, T>(Expression::Atan2<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> atan2float(0xbd26dbf7 /* "atan2" */, ConfigureAtan2<float>);
static ExpressionConfigure::Auto<__m128> atan2vector(0xbd26dbf7 /* "atan2" */, ConfigureAtan2<__m128>);

//
template<typename T> static void ConfigureSinh(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sinh<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> sinhfloat(0x10d2583f /* "sinh" */, ConfigureSinh<float>);
static ExpressionConfigure::Auto<__m128> sinhvector(0x10d2583f /* "sinh" */, ConfigureSinh<__m128>);

//
template<typename T> static void ConfigureCosh(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Cosh<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> coshfloat(0xf45c461c /* "cosh" */, ConfigureCosh<float>);
static ExpressionConfigure::Auto<__m128> coshvector(0xf45c461c /* "cosh" */, ConfigureCosh<__m128>);

//
template<typename T> static void ConfigureTanh(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Tanh<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> tanhfloat(0x092855d0 /* "tanh" */, ConfigureTanh<float>);
static ExpressionConfigure::Auto<__m128> tanhvector(0x092855d0 /* "tanh" */, ConfigureTanh<__m128>);

//
template<typename T> static void ConfigurePow(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureBinary<T, T, T>(Expression::Pow<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> powfloat(0x58336ad5 /* "pow" */, ConfigurePow<float>);
static ExpressionConfigure::Auto<__m128> powvector(0x58336ad5 /* "pow" */, ConfigurePow<__m128>);

//
template<typename T> static void ConfigureExp(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Exp<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> expfloat(0x72a68728 /* "exp" */, ConfigureExp<float>);
static ExpressionConfigure::Auto<__m128> expvector(0x72a68728 /* "exp" */, ConfigureExp<__m128>);

//
template<typename T> static void ConfigureLog(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Log<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> logfloat(0x3f515151 /* "log" */, ConfigureLog<float>);
static ExpressionConfigure::Auto<__m128> logvector(0x3f515151 /* "log" */, ConfigureLog<__m128>);

//
template<typename T> static void ConfigureSqrt(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sqrt<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> sqrtfloat(0x7dee3bcf /* "sqrt" */, ConfigureSqrt<float>);
static ExpressionConfigure::Auto<__m128> sqrtvector(0x7dee3bcf /* "sqrt" */, ConfigureSqrt<__m128>);

//
template<typename T> static void ConfigureInvSqrt(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::InvSqrt<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> invsqrtfloat(0x0a6a5946 /* "invsqrt" */, ConfigureInvSqrt<float>);
static ExpressionConfigure::Auto<__m128> invsqrtvector(0x0a6a5946 /* "invsqrt" */, ConfigureInvSqrt<__m128>);

//
template<typename T> static void ConfigureAbs(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Abs<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> absfloat(0x2a48023b /* "abs" */, ConfigureAbs<float>);
static ExpressionConfigure::Auto<__m128> absvector(0x2a48023b /* "abs" */, ConfigureAbs<__m128>);

//
template<typename T> static void ConfigureSign(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Sign<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> signfloat(0x0cbc8ba4 /* "sign" */, ConfigureSign<float>);
static ExpressionConfigure::Auto<__m128> signvector(0x0cbc8ba4 /* "sign" */, ConfigureSign<__m128>);

//
template<typename T> static void ConfigureFloor(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Floor<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> floorfloat(0xb8e70c1d /* "floor" */, ConfigureFloor<float>);
static ExpressionConfigure::Auto<__m128> floorvector(0xb8e70c1d /* "floor" */, ConfigureFloor<__m128>);

//
template<typename T> static void ConfigureCeil(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Ceil<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> ceilfloat(0x62e4e208 /* "ceil" */, ConfigureCeil<float>);
static ExpressionConfigure::Auto<__m128> ceilvector(0x62e4e208 /* "ceil" */, ConfigureCeil<__m128>);

//
template<typename T> static void ConfigureFrac(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureUnary<T, T>(Expression::Frac<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> fracfloat(0x87aad829 /* "frac" */, ConfigureFrac<float>);
static ExpressionConfigure::Auto<__m128> fracvector(0x87aad829 /* "frac" */, ConfigureFrac<__m128>);

//
template<typename T> static void ConfigureMod(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureBinary<T, T, T>(Expression::Mod<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> modfloat(0xdf9e7283 /* "mod" */, ConfigureMod<float>);
static ExpressionConfigure::Auto<__m128> modvector(0xdf9e7283 /* "mod" */, ConfigureMod<__m128>);

//
template<typename T> static void ConfigureMin(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Min<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> minfloat(0xc98f4557 /* "min" */, ConfigureMin<float>);
static ExpressionConfigure::Auto<__m128> minvector(0xc98f4557 /* "min" */, ConfigureMin<__m128>);

//
template<typename T> static void ConfigureMax(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureVariadic<T, T>(Expression::Max<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> maxfloat(0xd7a2e319 /* "max" */, ConfigureMax<float>);
static ExpressionConfigure::Auto<__m128> maxvector(0xd7a2e319 /* "max" */, ConfigureMax<__m128>);

//
template<typename T> static void ConfigureClamp(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureTernary<T, T, T, T>(Expression::Clamp<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> clampfloat(0xa82efcbc /* "clamp" */, ConfigureClamp<float>);
static ExpressionConfigure::Auto<__m128> clampvector(0xa82efcbc /* "clamp" */, ConfigureClamp<__m128>);

//
template<typename T> static void ConfigureLerp(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureTernary<T, T, T, float>(Expression::Lerp<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> lerpfloat(0x1e691468 /* "lerp" */, ConfigureLerp<float>);
static ExpressionConfigure::Auto<__m128> lerpvector(0x1e691468 /* "lerp" */, ConfigureLerp<__m128>);

//
template<typename T> static void ConfigureStep(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureTernary<T, T, T, float>(Expression::Step<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> stepfloat(0xc7441a0f /* "step" */, ConfigureStep<float>);
static ExpressionConfigure::Auto<__m128> stepvector(0xc7441a0f /* "step" */, ConfigureStep<__m128>);

//
template<typename T> static void ConfigureSmoothStep(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[])
{
	ConfigureTernary<T, T, T, float>(Expression::SmoothStep<T>, element, buffer, names, data);
}

static ExpressionConfigure::Auto<float> smoothstepfloat(0x95964e7d /* "smoothstep" */, ConfigureSmoothStep<float>);
static ExpressionConfigure::Auto<__m128> smoothstepvector(0x95964e7d /* "smoothstep" */, ConfigureSmoothStep<__m128>);
