#include "StdAfx.h"

#include "ExpressionExtend.h"
#include "ExpressionConfigure.h"

static void ConfigureExtend(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureUnary<__m128, float, Expression::Context &>(Expression::Extend, element, buffer, sScalarNames, sScalarDefault);
}

static Expression::Loader<__m128> extendvector(0xaa7d7949 /* "extend" */, ConfigureExtend);

namespace Expression
{
	__m128 Extend(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return _mm_set_ps1(arg);
	}
}
