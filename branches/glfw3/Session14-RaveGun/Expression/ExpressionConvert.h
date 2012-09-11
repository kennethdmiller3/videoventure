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

	// float to vector2: extend
	template <> struct Convert<Vector2, float>
	{
		static void Append(std::vector<unsigned int> &buffer)
		{
			Expression::Append(buffer, Expression::Extend<Vector2, float>);
		}
	};

	// float to vector3: extend
	template <> struct Convert<Vector3, float>
	{
		static void Append(std::vector<unsigned int> &buffer)
		{
			Expression::Append(buffer, Expression::Extend<Vector3, float>);
		}
	};

	// float to vector4: extend
	template <> struct Convert<Vector4, float>
	{
		static void Append(std::vector<unsigned int> &buffer)
		{
			Expression::Append(buffer, Expression::Extend<Vector4, float>);
		}
	};

	// float to color4: extend (HACK)
	template <> struct Convert<Color4, float>
	{
		static void Append(std::vector<unsigned int> &buffer)
		{
			Expression::Append(buffer, Expression::Extend<Color4, float>);
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
template <typename T, typename A> void ConfigureConvert(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	DebugPrint("%s convert %s\n", Expression::Schema<T>::NAME, Expression::Schema<A>::NAME);

	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for type conversion");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Convert<T, A>::Append(buffer);

	// append first argument
	ConfigureExpression<A>(arg1, buffer, names, defaults);
}
