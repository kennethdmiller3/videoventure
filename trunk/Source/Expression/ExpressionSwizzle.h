#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"

//
// SWIZZLE EXPRESSION
// shuffles components of a vector
//

namespace Expression
{
	// swizzle operators
	template <typename T> const T Swizzle(Context &aContext);
	template <> GAME_API const float Swizzle<float>(Context &aContext);
	template <> GAME_API const Vector2 Swizzle<Vector2>(Context &aContext);
	template <> GAME_API const Vector3 Swizzle<Vector3>(Context &aContext);
	template <> GAME_API const Vector4 Swizzle<Vector4>(Context &aContext);
	template <> GAME_API const Color4 Swizzle<Color4>(Context &aContext);
	template <> GAME_API const __m128 Swizzle<__m128>(Context &aContext);
}

// configure swizzle expression
template <typename T> void ConfigureSwizzle(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s swizzle\n", Expression::Schema<T>::NAME);
#endif

	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for swizzle operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// append the operator
	Expression::Append(buffer, Expression::Swizzle<T>);

	// for each component...
	unsigned char map[width];
	for (int i = 0; i < width; ++i)
	{
		// get mapping
		const char *attrib = element->Attribute(names[i]);
		if (attrib == NULL)
		{
			map[i] = unsigned char(i);
		}
		else if (isdigit(attrib[0]))
		{
			map[i] = attrib[0] - '0';
		}
		else if (unsigned int hash = Hash(attrib))
		{
			for (int j = 0; j < width; ++j)
			{
				if (Hash(names[j]) == hash)
				{
					map[i] = unsigned char(j);
					break;
				}
			}
		}
	}

	// append map
	assert(width <= 4);
	Expression::Append(buffer, *reinterpret_cast<unsigned int *>(map));

	// append first argument
	ConfigureExpression<T>(arg1, buffer, names, defaults);
}
