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
	template <> GAME_API const float Swizzle<float>(Context &aContext);
	template <> GAME_API const Vector2 Swizzle<Vector2>(Context &aContext);
	template <> GAME_API const Vector3 Swizzle<Vector3>(Context &aContext);
	template <> GAME_API const Vector4 Swizzle<Vector4>(Context &aContext);
	template <> GAME_API const Color4 Swizzle<Color4>(Context &aContext);
	template <> GAME_API const __m128 Swizzle<__m128>(Context &aContext);
}

// configure swizzle expression
template <typename T> void ConfigureSwizzle(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
