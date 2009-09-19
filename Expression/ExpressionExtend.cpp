#include "StdAfx.h"

#include "ExpressionExtend.h"

namespace Expression
{
	template <> const float Extend<float, float>(Context &aContext)
	{
		return Evaluate<float>(aContext);
	}
	template <> const Vector2 Extend<Vector2, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector2(arg, arg);
	}
	template <> const Vector3 Extend<Vector3, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector3(arg, arg, arg);
	}
	template <> const Vector4 Extend<Vector4, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector4(arg, arg, arg, arg);
	}
	template <> const Color4 Extend<Color4, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Color4(arg, arg, arg, arg);
	}
	template <> const __m128 Extend<__m128, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return _mm_set_ps1(arg);
	}
}