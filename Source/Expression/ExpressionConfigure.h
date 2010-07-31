#pragma once

#include "ExpressionLiteral.h"
#include "ExpressionVariable.h"
#include "ExpressionInterpolator.h"
#include "ExpressionRandom.h"
#include "ExpressionConstruct.h"
#include "ExpressionBoolean.h"

extern GAME_API const char * const sScalarNames[];
extern GAME_API const float sScalarDefault[];

//
// EXPRESSION


namespace ExpressionConfigure
{
	typedef fastdelegate::FastDelegate<void (const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])> Entry;

	template <typename T> Database::Typed<Entry> &GetDB()
	{
		static Database::Typed<Entry> configure;
		return configure;
	}
	template <typename T> void Add(unsigned int aTagId, Entry aConfigure)
	{
		GetDB<T>().Put(aTagId, aConfigure);
	}
	template <typename T> const Entry &Get(unsigned int aTagId)
	{
		return GetDB<T>().Get(aTagId);
	}

	template <typename T> struct Auto
	{
		Auto(unsigned int aTagId, Entry aEntry)
		{
			Add<T>(aTagId, aEntry);
		}
	};
}

//
// configure an expression
template <typename T> void ConfigureExpression(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s expression %s\n", Expression::Schema<T>::NAME, element->Value());
#endif

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// get hash of tag name
	unsigned int hash = Hash(element->Value());

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == TIXML_SUCCESS)
			overrided = true;
	}

	// if the tag matches a configure database entry...
	const ExpressionConfigure::Entry &entry = ExpressionConfigure::Get<T>(hash);
	if (entry)
	{
		// use the entry
		entry(element, buffer, names, data);
		return;
	}

	// default to tag variable
	ConfigureTagVariable<T>(element, buffer, names, data);

#if 0
	// configure based on tag name
	switch(hash)
	{
	case 0x425ed3ca /* "value" */:			ConfigureLiteral<T>(element, buffer, names, data); return;
	case 0x19385305 /* "variable" */:		ConfigureVariable<T>(element, buffer, names, data); return;
	case 0x29df7ff5 /* "resource" */:		Expression::Convert<T, float>::Append(buffer); ConfigureResource(element, buffer, sScalarNames, sScalarDefault); return;
	case 0x83588fd4 /* "interpolator" */:	ConfigureInterpolator<T>(element, buffer, names, data); return;
	case 0xa19b8cd6 /* "rand" */:			ConfigureRandom<T>(element, buffer, names, data); return;
	case 0x904416d1 /* "noise" */:			Expression::Convert<T, float>::Append(buffer); ConfigureNoise(element, buffer, names, data); return;

	case 0xaa7d7949 /* "extend" */:			ConfigureUnary<const T, float, Expression::Context &>(Expression::Extend<T, float>, element, buffer, sScalarNames, sScalarDefault); return;
	case 0x40c09172 /* "construct" */:		ConfigureConstruct<T>(element, buffer, names, data); return;
	case 0x3deb1461 /* "swizzle" */:		ConfigureSwizzle<T>(element, buffer, names, data); return;
	case 0xf667bf8a /* "worldtime" */:		Expression::Convert<T, float>::Append(buffer); Expression::Append(buffer, EvaluateWorldTime); return;
	case 0x5d3c9be4 /* "time" */:			Expression::Convert<T, float>::Append(buffer); Expression::Append(buffer, EvaluateTime); return;

	case 0x3b391274 /* "add" */:			ConfigureVariadic<T, T>(Expression::Add<T>, element, buffer, names, data); return;
	case 0xdc4e3915 /* "sub" */:			ConfigureVariadic<T, T>(Expression::Sub<T>, element, buffer, names, data); return;
	case 0xeb84ed81 /* "mul" */:			ConfigureVariadic<T, T>(Expression::Mul<T>, element, buffer, names, data); return;
	case 0xe562ab48 /* "div" */:			ConfigureVariadic<T, T>(Expression::Div<T>, element, buffer, names, data); return;
	case 0x3899af41 /* "neg" */:			ConfigureUnary<T, T>(Expression::Neg<T>, element, buffer, names, data); return;
	case 0x31037236 /* "rcp" */:			ConfigureUnary<T, T>(Expression::Rcp<T>, element, buffer, names, data); return;
	case 0xa8e99c47 /* "inc" */:			ConfigureUnary<T, T>(Expression::Inc<T>, element, buffer, names, data); return;
	case 0xc25979d3 /* "dec" */:			ConfigureUnary<T, T>(Expression::Dec<T>, element, buffer, names, data); return;
									
	case 0xe0302a4d /* "sin" */:			ConfigureUnary<T, T>(Expression::Sin<T>, element, buffer, names, data); return;
	case 0xfb8de29c /* "cos" */:			ConfigureUnary<T, T>(Expression::Cos<T>, element, buffer, names, data); return;
	case 0x9cf73498 /* "tan" */:			ConfigureUnary<T, T>(Expression::Tan<T>, element, buffer, names, data); return;
	case 0xfeae7ea6 /* "asin" */:			ConfigureUnary<T, T>(Expression::Asin<T>, element, buffer, names, data); return;
	case 0x3c01df1f /* "acos" */:			ConfigureUnary<T, T>(Expression::Acos<T>, element, buffer, names, data); return;
	case 0x0678cabf /* "atan" */:			ConfigureUnary<T, T>(Expression::Atan<T>, element, buffer, names, data); return;
	case 0xbd26dbf7 /* "atan2" */:			ConfigureBinary<T, T, T>(Expression::Atan2<T>, element, buffer, names, data); return;
									
	case 0x10d2583f /* "sinh" */:			ConfigureUnary<T, T>(Expression::Sinh<T>, element, buffer, names, data); return;
	case 0xf45c461c /* "cosh" */:			ConfigureUnary<T, T>(Expression::Cosh<T>, element, buffer, names, data); return;
	case 0x092855d0 /* "tanh" */:			ConfigureUnary<T, T>(Expression::Tanh<T>, element, buffer, names, data); return;
									
	case 0x58336ad5 /* "pow" */:			ConfigureBinary<T, T, T>(Expression::Pow<T>, element, buffer, names, data); return;
	case 0x72a68728 /* "exp" */:			ConfigureUnary<T, T>(Expression::Exp<T>, element, buffer, names, data); return;
	case 0x3f515151 /* "log" */:			ConfigureUnary<T, T>(Expression::Log<T>, element, buffer, names, data); return;
	case 0x7dee3bcf /* "sqrt" */:			ConfigureUnary<T, T>(Expression::Sqrt<T>, element, buffer, names, data); return;
	case 0x0a6a5946 /* "invsqrt" */:		ConfigureUnary<T, T>(Expression::InvSqrt<T>, element, buffer, names, data); return;
									
	case 0x2a48023b /* "abs" */:			ConfigureUnary<T, T>(Expression::Abs<T>, element, buffer, names, data); return;
	case 0x0cbc8ba4 /* "sign" */:			ConfigureUnary<T, T>(Expression::Sign<T>, element, buffer, names, data); return;
	case 0xb8e70c1d /* "floor" */:			ConfigureUnary<T, T>(Expression::Floor<T>, element, buffer, names, data); return;
	case 0x62e4e208 /* "ceil" */:			ConfigureUnary<T, T>(Expression::Ceil<T>, element, buffer, names, data); return;
	case 0x87aad829 /* "frac" */:			ConfigureUnary<T, T>(Expression::Frac<T>, element, buffer, names, data); return;
	case 0xdf9e7283 /* "mod" */:			ConfigureBinary<T, T, T>(Expression::Mod<T>, element, buffer, names, data); return;
	case 0xc98f4557 /* "min" */:			ConfigureVariadic<T, T>(Expression::Min<T>, element, buffer, names, data); return;
	case 0xd7a2e319 /* "max" */:			ConfigureVariadic<T, T>(Expression::Max<T>, element, buffer, names, data); return;
	case 0xa82efcbc /* "clamp" */:			ConfigureTernary<T, T, T, T>(Expression::Clamp<T>, element, buffer, names, data); return;
	case 0x1e691468 /* "lerp" */:			ConfigureTernary<T, T, T, float>(Expression::Lerp<T>, element, buffer, names, data); return;
	case 0xc7441a0f /* "step" */:			ConfigureBinary<T, T, T>(Expression::Step<T>, element, buffer, names, data); return;
	case 0x95964e7d /* "smoothstep" */:		ConfigureTernary<T, T, T, T>(Expression::SmoothStep<T>, element, buffer, names, data); return;

	case 0xb711f539 /* "sinewave" */:		Expression::Convert<T, float>::Append(buffer); ConfigureSineWave(element, buffer); return;
	case 0xd0308494 /* "trianglewave" */:	Expression::Convert<T, float>::Append(buffer); ConfigureTriangleWave(element, buffer); return;
	case 0x705614d5 /* "sawtoothwave" */:	Expression::Convert<T, float>::Append(buffer); ConfigureSawtoothWave(element, buffer); return;
	case 0x3f8dc467 /* "pulsewave" */:		Expression::Convert<T, float>::Append(buffer); ConfigurePulseWave(element, buffer); return;

	default:								ConfigureTagVariable<T>(element, buffer, names, data); return;
	}
#endif
}

// configure an expression root (the tag hosting the expression)
template <typename T> void ConfigureExpressionRoot(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s root %s\n", Expression::Schema<T>::NAME, element->Value());
#endif

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == TIXML_SUCCESS)
			overrided = true;
	}

	// special case: attribute variable reference
	if (element->Attribute("variable"))
	{
		ConfigureInlineVariable<T>(element, buffer, names, data);
		return;
	}

	// special case: attribute random value
	if (element->Attribute("rand"))
	{
		ConfigureRandom<T>(element, buffer, names, data);
		return;
	}

	// special case: embedded interpolator keyframes
	if (element->FirstChildElement("key"))
	{
		ConfigureInterpolator<T>(element, buffer, names, data);
		return;
	}

	// special case: no child elements
	if (!element->FirstChildElement())
	{
		// push literal data
		ConfigureLiteral<T>(element, buffer, names, data);
		return;
	}

	// special case: component elements
	for (int i = 0; i < width; ++i)
	{
		if (element->FirstChildElement(names[i]))
		{
			ConfigureConstruct<T>(element, buffer, names, data);
			return;
		}
	}

	// for each child node...
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		// recurse on child
		ConfigureExpression<T>(child, buffer, names, data);
	}
}
