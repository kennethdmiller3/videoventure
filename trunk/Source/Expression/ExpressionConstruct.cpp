#include "StdAfx.h"

#include "ExpressionConstruct.h"
#include "ExpressionConfigure.h"

template <> void ConfigureConstruct<float>(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// straight passthrough
}

template <> void ConfigureConstruct<bool>(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// cast
	Expression::Append(buffer, Expression::Construct<bool, float>);
}

static Expression::Loader<__m128> constructvector(0x40c09172 /* "construct" */, ConfigureConstruct<__m128>);



// configure construct expression
template <typename T> void ConfigureConstruct(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s construct\n", Expression::Schema<T>::NAME);
#endif

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// append the operator
	Expression::Append(buffer, Expression::Construct<T, float>);

	// for each component...
	for (int i = 0; i < width; ++i)
	{
		// if there is a corresponding tag...
		if (const tinyxml2::XMLElement *component = element->FirstChildElement(names[i]))
		{
			// configure the expression
			Expression::Loader<float>::ConfigureRoot(component, buffer, sScalarNames, &defaults[i]);
		}
		else
		{
			// use default value
#ifdef PRINT_CONFIGURE_EXPRESSION
			DebugPrint("%s default %s: %f\n", Expression::Schema<float>::NAME, names[i], defaults[i]);
#endif
			Expression::Append(buffer, Expression::Constant<float>, defaults[i]);
		}
	}
}

namespace Expression
{
	// constructors
	template <> bool Construct<bool, float>(Context &aContext)
	{
		return Evaluate<float>(aContext) != 0.0f;
	}
	template <> __m128 Construct<__m128, float>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return _mm_setr_ps(arg1, arg2, arg3, arg4);
	}
}
