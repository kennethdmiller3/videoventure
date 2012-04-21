#include "StdAfx.h"

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionLiteral.h"
#include "ExpressionConvert.h"
#include "ExpressionConfigure.h"
#include "ExpressionNoise.h"
#include "Noise.h"

template<typename T> static void ConfigureNoise(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	Expression::Convert<T, float>::Append(buffer);
	ConfigureNoise(element, buffer, names, defaults);
}

static Expression::Loader<float>::Auto noisefloat(0x904416d1 /* "noise" */, ConfigureNoise<float>);
static Expression::Loader<__m128>::Auto noisevector(0x904416d1 /* "noise" */, ConfigureNoise<__m128>);

// configue a parameter
static bool ConfigureParameter(const tinyxml2::XMLElement *element, const char *param, std::vector<unsigned int> &buffer, float (*op)(Expression::Context &), const char * const names[], const float defvalue, const float identity)
{
	// get constant value from attribute
	float value = defvalue;
	element->QueryFloatAttribute(param, &value);

	// if there is a child tag for the parameter...
	if (const tinyxml2::XMLElement *child = element->FirstChildElement(param))
	{
		// append the operation (if any)
		if (op)
			Expression::Append(buffer, op);

		// configure the expression
		Expression::Loader<float>::ConfigureRoot(child, buffer, names, &value);
		return true;
	}

	// if the value is not identity...
	if (value != identity)
	{
		// append the operation (if any)
		if (op)
			Expression::Append(buffer, op);

		// append a constant expression
		Expression::Append(buffer, Expression::Constant<float>, value);
		return true;
	}

	// no parameter found
	return false;
}

//
// NOISE EXPRESSION
//
void ConfigureNoise(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// configure offset
	ConfigureParameter(element, "offset", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// configure scale
	ConfigureParameter(element, "scale", buffer, Expression::Mul<float>, sScalarNames, 1.0f, 1.0f);

	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
#ifdef PRINT_CONFIGURE_EXPRESSION
		DebugPrint("%s noise ->", Expression::Schema<float>::NAME);
#endif
		ConfigureLiteral<float>(element, buffer, names, defaults);
		return;
	}

	const tinyxml2::XMLElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
#ifdef PRINT_CONFIGURE_EXPRESSION
		DebugPrint("%s noise1\n", Expression::Schema<float>::NAME);
#endif
		Expression::Append(buffer, Expression::ComponentUnary<float, Expression::Schema<float>::COUNT>::Evaluate<float, float, Noise>);
		Expression::Loader<float>::Configure(arg1, buffer, names, defaults);
		return;
	}

	const tinyxml2::XMLElement *arg3 = arg2->NextSiblingElement();
	if (!arg3)
	{
#ifdef PRINT_CONFIGURE_EXPRESSION
		DebugPrint("%s noise2\n", Expression::Schema<float>::NAME);
#endif
		Expression::Append(buffer, Expression::ComponentBinary<float, Expression::Schema<float>::COUNT>::Evaluate<float, float, float, Noise>);
		Expression::Loader<float>::Configure(arg1, buffer, names, defaults);
		Expression::Loader<float>::Configure(arg2, buffer, names, defaults);
		return;
	}

	const tinyxml2::XMLElement *arg4 = arg3->NextSiblingElement();
	if (!arg4)
	{
#ifdef PRINT_CONFIGURE_EXPRESSION
		DebugPrint("%s noise3\n", Expression::Schema<float>::NAME);
#endif
		Expression::Append(buffer, Expression::ComponentTernary<float, Expression::Schema<float>::COUNT>::Evaluate<float, float, float, float, Noise>);
		Expression::Loader<float>::Configure(arg1, buffer, names, defaults);
		Expression::Loader<float>::Configure(arg2, buffer, names, defaults);
		Expression::Loader<float>::Configure(arg3, buffer, names, defaults);
		return;
	}

#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s noise4\n", Expression::Schema<float>::NAME);
#endif
	assert(false);
}
