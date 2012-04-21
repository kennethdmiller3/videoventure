#include "StdAfx.h"

#include "ExpressionLiteral.h"
#include "ExpressionConfigure.h"

static Expression::Loader<float>::Auto literalfloat(0x425ed3ca /* "value" */, ConfigureLiteral<float>);
static Expression::Loader<__m128>::Auto literalvector(0x425ed3ca /* "value" */, ConfigureLiteral<__m128>);

//
// LITERAL EXPRESSION
// returns an embedded constant value
//

// float[width] literal
void ConfigureLiteral(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[])
{
	// process literal data
	for (int i = 0; i < width; ++i)
	{
		float value = defaults[i];
		element->QueryFloatAttribute(names[i], &value);
		Expression::Append(buffer, value);
#ifdef PRINT_CONFIGURE_EXPRESSION
		DebugPrint(" %s=%f", names[i], value);
#endif
	}
}
