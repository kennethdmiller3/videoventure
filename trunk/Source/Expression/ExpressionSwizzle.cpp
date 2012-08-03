#include "StdAfx.h"

#include "ExpressionSwizzle.h"
#include "ExpressionConfigure.h"


static Expression::Loader<__m128> swizzlevector(0x3deb1461 /* "swizzle" */, ConfigureSwizzle);

namespace Expression
{
	struct SwizzleMap { unsigned char c[4]; };
	__m128 Swizzle(Context &aContext)
	{
		const SwizzleMap map(Read<SwizzleMap>(aContext));
		const __m128 arg(Evaluate<__m128>(aContext));
		__m128 ret = _mm_setzero_ps();
		for (register int i = 0; i < 4; ++i)
			ret.m128_f32[i] = arg.m128_f32[map.c[i]];
		return ret;
	}
}

// configure swizzle expression
void ConfigureSwizzle(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s swizzle\n", Expression::Schema<__m128>::NAME);
#endif

	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for swizzle operator");
		ConfigureLiteral<__m128>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, Expression::Swizzle);

	// for each component...
	Expression::SwizzleMap map = { 0 };
	for (int i = 0; i < 4; ++i)
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
			for (int j = 0; j < 4; ++j)
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
	Expression::Loader<__m128>::Configure(arg1, buffer, names, defaults);
}
