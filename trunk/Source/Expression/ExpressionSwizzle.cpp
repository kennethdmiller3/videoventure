#include "StdAfx.h"

#include "ExpressionSwizzle.h"
#include "ExpressionConfigure.h"


static Expression::Loader<float>::Auto swizzlefloat(0x3deb1461 /* "swizzle" */, ConfigureSwizzle<float>);
static Expression::Loader<__m128>::Auto swizzlevector(0x3deb1461 /* "swizzle" */, ConfigureSwizzle<__m128>);

namespace Expression
{
	union SwizzleMap { unsigned int u; unsigned char c[4]; };
	template <> const float Swizzle<float>(Context &aContext)
	{
		return Evaluate<float>(aContext);
	}
	template <> const Vector2 Swizzle<Vector2>(Context &aContext)
	{
		Vector2 arg(Evaluate<Vector2>(aContext));
		SwizzleMap map;
		map.u = Read<unsigned int>(aContext);
		return Vector2(arg[map.c[0]], arg[map.c[1]]);
	}
	template <> const Vector3 Swizzle<Vector3>(Context &aContext)
	{
		Vector3 arg(Evaluate<Vector3>(aContext));
		SwizzleMap map;
		map.u = Read<unsigned int>(aContext);
		return Vector3(arg[map.c[0]], arg[map.c[1]], arg[map.c[2]]);
	}
	template <> const Vector4 Swizzle<Vector4>(Context &aContext)
	{
		Vector4 arg(Evaluate<Vector4>(aContext));
		SwizzleMap map;
		map.u = Read<unsigned int>(aContext);
		return Vector4(arg[map.c[0]], arg[map.c[1]], arg[map.c[2]], arg[map.c[3]]);
	}
	template <> const Color4 Swizzle<Color4>(Context &aContext)
	{
		Color4 arg(Evaluate<Color4>(aContext));
		SwizzleMap map;
		map.u = Read<unsigned int>(aContext);
		return Color4(arg[map.c[0]], arg[map.c[1]], arg[map.c[2]], arg[map.c[3]]);
	}
	template <> const __m128 Swizzle<__m128>(Context &aContext)
	{
		SwizzleMap map;
		map.u = Read<unsigned int>(aContext);
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
	Expression::Loader<T>::Configure(arg1, buffer, names, defaults);
}

