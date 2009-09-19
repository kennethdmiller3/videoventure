#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"

//
// CONSTRUCT EXPRESSION
// build a vector from scalar components
//

namespace Expression
{
	// constructors
	template <> const bool Construct<bool, float>(Context &aContext);
	template <> const float Construct<float, float>(Context &aContext);
	template <> const Vector2 Construct<Vector2, float>(Context &aContext);
	template <> const Vector3 Construct<Vector3, float>(Context &aContext);
	template <> const Vector4 Construct<Vector4, float>(Context &aContext);
	template <> const Color4 Construct<Color4, float>(Context &aContext);
	template <> const __m128 Construct<__m128, float>(Context &aContext);
}

// configure construct expression
template <typename T> void ConfigureConstruct(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	DebugPrint("%s construct\n", Expression::Schema<T>::NAME);

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// append the operator
	Expression::Append(buffer, Expression::Construct<T, float>);

	// for each component...
	for (int i = 0; i < width; ++i)
	{
		// if there is a corresponding tag...
		if (const TiXmlElement *component = element->FirstChildElement(names[i]))
		{
			// configure the expression
			ConfigureExpressionRoot<float>(component, buffer, sScalarNames, &defaults[i]);
		}
		else
		{
			// use default value
			DebugPrint("%s default %s: %f\n", Expression::Schema<float>::NAME, names[i], defaults[i]);
			Expression::Append(buffer, Expression::Constant<float>, defaults[i]);
		}
	}
}
