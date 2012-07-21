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
	template <> GAME_API const bool Construct<bool, float>(Context &aContext);
	template <> GAME_API const float Construct<float, float>(Context &aContext);
	template <> GAME_API const Vector2 Construct<Vector2, float>(Context &aContext);
	template <> GAME_API const Vector3 Construct<Vector3, float>(Context &aContext);
	template <> GAME_API const Vector4 Construct<Vector4, float>(Context &aContext);
	template <> GAME_API const Color4 Construct<Color4, float>(Context &aContext);
	template <> GAME_API const __m128 Construct<__m128, float>(Context &aContext);
}

// configure construct expression
template <typename T> void ConfigureConstruct(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
