#include "StdAfx.h"

#include "ExpressionExtend.h"
#include "ExpressionConfigure.h"

template<typename T> static void ConfigureExtend(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureUnary<T, float, Expression::Context &>(Expression::Extend<T, float>, element, buffer, sScalarNames, sScalarDefault);
}

static Expression::Loader<__m128> extendvector(0xaa7d7949 /* "extend" */, ConfigureExtend<__m128>);

namespace Expression
{
	template <> __m128 Extend<__m128, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return _mm_set_ps1(arg);
	}
}
