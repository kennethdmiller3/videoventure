#include "StdAfx.h"

#include "ExpressionLiteral.h"
#include "ExpressionConfigure.h"

static ExpressionConfigure::Auto<float> literalfloat(0x425ed3ca /* "value" */, ConfigureLiteral<float>);
static ExpressionConfigure::Auto<__m128> literalvector(0x425ed3ca /* "value" */, ConfigureLiteral<__m128>);

//
// LITERAL EXPRESSION
// returns an embedded constant value
//

// float[width] literal
void ConfigureLiteral(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[])
{
	// process literal data
	for (int i = 0; i < width; ++i)
	{
		float value = defaults[i];
		element->QueryFloatAttribute(names[i], &value);
		buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
#ifdef PRINT_CONFIGURE_EXPRESSION
		DebugPrint(" %s=%f", names[i], value);
#endif
	}
}