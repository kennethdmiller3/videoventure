#include "StdAfx.h"

#include "Expression.h"
#include "ExpressionLogical.h"
#include "ExpressionRelational.h"
#include "ExpressionOperator.h"
#include "ExpressionConvert.h"
#include "ExpressionConfigure.h"

#include "ExpressionBoolean.h"

static void ConfigureBooleanVariadic(bool (*expr)(Expression::Context &), const TiXmlElement *arg1, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	if (const TiXmlElement *arg2 = arg1->NextSiblingElement())
	{
		Expression::Append(buffer, expr);
		buffer.push_back(0);
		int start = buffer.size();
		ConfigureExpression<bool>(arg1, buffer, names, defaults);
		ConfigureBooleanVariadic(expr, arg2, buffer, names, defaults);
		buffer[start - 1] = buffer.size() - start;
	}
	else
	{
		ConfigureExpression<bool>(arg1, buffer, names, defaults);
	}
}

static void ConfigureAnd(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		DebugPrint("no first argument for logical and");
		Expression::Append(buffer, Expression::Constant<bool>, false);
		return;
	}

	ConfigureBooleanVariadic(Expression::And, arg1, buffer, names, defaults);
}
static ExpressionConfigure::Auto<bool> andbool(0x0f29c2a6 /* "and" */, ConfigureAnd);

static void ConfigureOr(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		DebugPrint("no first argument for logical or");
		Expression::Append(buffer, Expression::Constant<bool>, false);
		return;
	}

	ConfigureBooleanVariadic(Expression::Or, arg1, buffer, names, defaults);
}
static ExpressionConfigure::Auto<bool> orbool(0x5d342984 /* "or" */, ConfigureOr);

static void ConfigureNot(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		DebugPrint("no first argument for logical not");
		Expression::Append(buffer, Expression::Constant<bool>, true);
	}

	Expression::Append(buffer, Expression::Not);
	ConfigureExpression<bool>(arg1, buffer, names, defaults);
}
static ExpressionConfigure::Auto<bool> notbool(0x29b19c8a /* "not" */, ConfigureNot);

static void ConfigureXor(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		DebugPrint("no first argument for logical xor");
		Expression::Append(buffer, Expression::Constant<bool>, false);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		DebugPrint("no second argument for logical xor");
		ConfigureExpression<bool>(arg1, buffer, names, defaults);
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
			Expression::Append(buffer, Expression::Xor);
		}

		// append first argument
		ConfigureExpression<bool>(arg1, buffer, names, defaults);
	}
	while (arg2);
}
static ExpressionConfigure::Auto<bool> xorbool(0xcc6bdb7e /* "xor" */, ConfigureXor);

static void ConfigureGreater(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float, Expression::Context &>(Expression::Greater<float>, element, buffer, sScalarNames, sScalarDefault);
}
static ExpressionConfigure::Auto<bool> greaterbool(0x50c80b99 /* "greater" */, ConfigureGreater);

static void ConfigureGreaterEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float, Expression::Context &>(Expression::GreaterEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static ExpressionConfigure::Auto<bool> greaterequalbool(0xf75208d3 /* "greaterequal" */, ConfigureGreaterEqual);

static void ConfigureLess(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float, Expression::Context &>(Expression::Less<float>, element, buffer, sScalarNames, sScalarDefault);
}
static ExpressionConfigure::Auto<bool> lessbool(0x216b57b8 /* "less" */, ConfigureLess);

static void ConfigureLessEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float, Expression::Context &>(Expression::LessEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static ExpressionConfigure::Auto<bool> lessequalbool(0xce1f56b0 /* "lessequal" */, ConfigureLessEqual);

static void ConfigureEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float, Expression::Context &>(Expression::Equal<float>, element, buffer, sScalarNames, sScalarDefault);
}
static ExpressionConfigure::Auto<bool> equalbool(0x2f7508ef /* "equal" */, ConfigureEqual);

static void ConfigureNotEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float, Expression::Context &>(Expression::NotEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static ExpressionConfigure::Auto<bool> notequalbool(0x7eca4a1e /* "notequal" */, ConfigureNotEqual);

template<> void ConfigureExpression<bool>(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	unsigned int hash = Hash(element->Value());

	const ExpressionConfigure::Entry &entry = ExpressionConfigure::Get<bool>(hash);
	if (entry)
	{
		entry(element, buffer, sScalarNames, sScalarDefault);
		return;
	}

	Expression::Convert<bool, float>::Append(buffer);
	ConfigureExpression<float>(element, buffer, sScalarNames, sScalarDefault);
}
