#pragma once

#include "Expression.h"

//
// EXTEND EXPRESSIONS
// extend a scalar type to a vector type
//

namespace Expression
{
	template <typename T, typename A> const T Extend(Context &aContext);
	template <> GAME_API const float Extend<float, float>(Context &aContext);
	template <> GAME_API const Vector2 Extend<Vector2, float>(Context &aContext);
	template <> GAME_API const Vector3 Extend<Vector3, float>(Context &aContext);
	template <> GAME_API const Vector4 Extend<Vector4, float>(Context &aContext);
	template <> GAME_API const Color4 Extend<Color4, float>(Context &aContext);
	template <> GAME_API const __m128 Extend<__m128, float>(Context &aContext);
}