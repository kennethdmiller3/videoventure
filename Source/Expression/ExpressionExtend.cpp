#include "StdAfx.h"

#include "ExpressionExtend.h"
#include "ExpressionConfigure.h"

template<typename T> static void ConfigureExtend(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureUnary<const T, float, Expression::Context &>(Expression::Extend<T, float>, element, buffer, sScalarNames, sScalarDefault);
}

static Expression::Loader<float>::Auto extendfloat(0xaa7d7949 /* "extend" */, ConfigureExtend<float>);
static Expression::Loader<__m128>::Auto extendvector(0xaa7d7949 /* "extend" */, ConfigureExtend<__m128>);

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
