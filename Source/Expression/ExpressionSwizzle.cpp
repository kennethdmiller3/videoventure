#include "StdAfx.h"

#include "ExpressionSwizzle.h"
#include "ExpressionConfigure.h"


static Expression::Loader<__m128> swizzlevector(0x3deb1461 /* "swizzle" */, ConfigureSwizzle<__m128>);

namespace Expression
{
	struct SwizzleMap { unsigned char c[4]; };
	template <> __m128 Swizzle<__m128>(Context &aContext)
	{
		const SwizzleMap map(Read<SwizzleMap>(aContext));
		__m128 arg(Evaluate<__m128>(aContext));
		return _mm_setr_ps(
			reinterpret_cast<float *>(&arg)[map.c[0]],
			reinterpret_cast<float *>(&arg)[map.c[1]],
			reinterpret_cast<float *>(&arg)[map.c[2]],
			reinterpret_cast<float *>(&arg)[map.c[3]]
		);
	}
}

// configure swizzle expression
template <typename T> void ConfigureSwizzle(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s swizzle\n", Expression::Schema<T>::NAME);
#endif

	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for swizzle operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);
	assert(width <= 4);

	// append the operator
	Expression::Append(buffer, Expression::Swizzle<T>);

	// for each component...
	Expression::SwizzleMap map = { 0 };
	for (int i = 0; i < width; ++i)
	{
		// get mapping
		const char *attrib = element->Attribute(names[i]);
		if (attrib == NULL)
		{
			map.c[i] = unsigned char(i);
		}
		else if (isdigit(attrib[0]))
		{
			map.c[i] = attrib[0] - '0';
		}
		else if (unsigned int hash = Hash(attrib))
		{
			for (int j = 0; j < width; ++j)
			{
				if (Hash(names[j]) == hash)
				{
					map.c[i] = unsigned char(j);
					break;
				}
			}
		}
	}

	// append map
	Expression::Append(buffer, map);

	// append first argument
	Expression::Loader<T>::Configure(arg1, buffer, names, defaults);
}

