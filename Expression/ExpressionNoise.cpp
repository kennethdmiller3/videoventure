#include "StdAfx.h"

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionLiteral.h"
#include "ExpressionConfigure.h"
#include "ExpressionNoise.h"
#include "Noise.h"

// configue a parameter
static bool ConfigureParameter(const TiXmlElement *element, const char *param, std::vector<unsigned int> &buffer, float (*op)(Expression::Context &), const char * const names[], const float defvalue, const float identity)
{
	// get constant value from attribute
	float value = defvalue;
	element->QueryFloatAttribute(param, &value);

	// if there is a child tag for the parameter...
	if (const TiXmlElement *child = element->FirstChildElement(param))
	{
		// append the operation (if any)
		if (op)
			Expression::Append(buffer, op);

		// configure the expression
		ConfigureExpressionRoot<float>(child, buffer, names, &value);
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
void ConfigureNoise(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// configure offset
	ConfigureParameter(element, "offset", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// configure scale
	ConfigureParameter(element, "scale", buffer, Expression::Mul<float>, sScalarNames, 1.0f, 1.0f);

	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		DebugPrint("%s noise ->", Expression::Schema<float>::NAME);
		ConfigureLiteral<float>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		DebugPrint("%s noise1\n", Expression::Schema<float>::NAME);
		Expression::Append(buffer, Expression::ComponentUnary<float, Expression::Schema<float>::COUNT>::Evaluate<float, float, Noise>);
		ConfigureExpression<float>(arg1, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg3 = arg2->NextSiblingElement();
	if (!arg3)
	{
		DebugPrint("%s noise2\n", Expression::Schema<float>::NAME);
		Expression::Append(buffer, Expression::ComponentBinary<float, Expression::Schema<float>::COUNT>::Evaluate<float, float, float, Noise>);
		ConfigureExpression<float>(arg1, buffer, names, defaults);
		ConfigureExpression<float>(arg2, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg4 = arg3->NextSiblingElement();
	if (!arg4)
	{
		DebugPrint("%s noise3\n", Expression::Schema<float>::NAME);
		Expression::Append(buffer, Expression::ComponentTernary<float, Expression::Schema<float>::COUNT>::Evaluate<float, float, float, float, Noise>);
		ConfigureExpression<float>(arg1, buffer, names, defaults);
		ConfigureExpression<float>(arg2, buffer, names, defaults);
		ConfigureExpression<float>(arg3, buffer, names, defaults);
		return;
	}

	DebugPrint("%s noise4\n", Expression::Schema<float>::NAME);
	assert(false);
}
