#include "StdAfx.h"

#include "ExpressionSwizzle.h"

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