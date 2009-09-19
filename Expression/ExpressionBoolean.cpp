#include "StdAfx.h"

#include "Expression.h"
#include "ExpressionConfigure.h"
#include "ExpressionLogical.h"
#include "ExpressionRelational.h"
#include "ExpressionOperator.h"

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


template<> void ConfigureExpression<bool>(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	switch(Hash(element->Value()))
	{
	case 0x0f29c2a6 /* "and" */:
		{
			const TiXmlElement *arg1 = element->FirstChildElement();
			if (!arg1)
			{
				DebugPrint("no first argument for logical and");
				Expression::Append(buffer, Expression::Constant<bool>, false);
				break;
			}

			ConfigureBooleanVariadic(Expression::And, arg1, buffer, names, defaults);
		}
		break;

	case 0x5d342984 /* "or" */:
		{
			const TiXmlElement *arg1 = element->FirstChildElement();
			if (!arg1)
			{
				DebugPrint("no first argument for logical and");
				Expression::Append(buffer, Expression::Constant<bool>, false);
				break;
			}

			ConfigureBooleanVariadic(Expression::Or, arg1, buffer, names, defaults);
		}
		break;

	case 0x29b19c8a /* "not" */:
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
		break;

	case 0xcc6bdb7e /* "xor" */:
		{
			const TiXmlElement *arg1 = element->FirstChildElement();
			if (!arg1)
			{
				DebugPrint("no first argument for logical xor");
				Expression::Append(buffer, Expression::Constant<bool>, false);
				break;
			}

			const TiXmlElement *arg2 = arg1->NextSiblingElement();
			if (!arg2)
			{
				DebugPrint("no second argument for logical xor");
				ConfigureExpression<bool>(arg1, buffer, names, defaults);
				break;
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
		break;

	case 0x50c80b99 /* "greater" */:
		ConfigureBinary<bool, float, float, Expression::Context &>(Expression::Greater<float>, element, buffer, sScalarNames, sScalarDefault);
		break;

	case 0xf75208d3 /* "greaterequal" */:
		ConfigureBinary<bool, float, float, Expression::Context &>(Expression::GreaterEqual<float>, element, buffer, sScalarNames, sScalarDefault);
		break;

	case 0x216b57b8 /* "less" */:
		ConfigureBinary<bool, float, float, Expression::Context &>(Expression::Less<float>, element, buffer, sScalarNames, sScalarDefault);
		break;

	case 0xce1f56b0 /* "lessequal" */:
		ConfigureBinary<bool, float, float, Expression::Context &>(Expression::LessEqual<float>, element, buffer, sScalarNames, sScalarDefault);
		break;

	case 0x2f7508ef /* "equal" */:
		ConfigureBinary<bool, float, float, Expression::Context &>(Expression::Equal<float>, element, buffer, sScalarNames, sScalarDefault);
		break;

	case 0x7eca4a1e /* "notequal" */:
		ConfigureBinary<bool, float, float, Expression::Context &>(Expression::NotEqual<float>, element, buffer, sScalarNames, sScalarDefault);
		break;

	default:
		Expression::Convert<bool, float>::Append(buffer); ConfigureExpression<float>(element, buffer, sScalarNames, sScalarDefault);
		break;
	}
}
