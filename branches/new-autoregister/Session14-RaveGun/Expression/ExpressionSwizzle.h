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
}

// configure swizzle expression
template <typename T> void ConfigureSwizzle(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	DebugPrint("%s swizzle\n", Expression::Schema<T>::NAME);

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
