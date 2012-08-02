#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionConstruct.h"
#include "ExpressionExtend.h"


namespace Expression
{
	//
	// TYPE CONVERSION
	//

	// generic conversion
	template <typename T, typename A> struct Convert
	{
		static void Append(std::vector<unsigned int> &buffer)
		{
			Expression::Append(buffer, Expression::Construct<T, A>);
		}
	};

	// no conversion
	template <typename T> struct Convert<T, T>
	{
		static void Append(std::vector<unsigned int> &buffer)
		{
		}
	};

	// float to SIMD: extend
	template <> struct Convert<__m128, float>
	{
		static void Append(std::vector<unsigned int> &buffer)
		{
			Expression::Append(buffer, Expression::Extend<__m128, float>);
		}
	};
}

// configure conversion
template <typename T, typename A> void ConfigureConvert(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
