#include "StdAfx.h"

#include "ExpressionConstruct.h"

namespace Expression
{
	// constructors
	template <> const bool Construct<bool, float>(Context &aContext)
	{
		return Evaluate<float>(aContext) != 0.0f;
	}
	template <> const float Construct<float, float>(Context &aContext)
	{
		return Evaluate<float>(aContext);
	}
	template <> const Vector2 Construct<Vector2, float>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		return Vector2(arg1, arg2);
	}
	template <> const Vector3 Construct<Vector3, float>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		return Vector3(arg1, arg2, arg3);
	}
	template <> const Vector4 Construct<Vector4, float>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return Vector4(arg1, arg2, arg3, arg4);
	}
	template <> const Color4 Construct<Color4, float>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return Color4(arg1, arg2, arg3, arg4);
	}
	template <> const __m128 Construct<__m128, float>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return _mm_setr_ps(arg1, arg2, arg3, arg4);
	}
}