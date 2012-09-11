#include "StdAfx.h"

#include "ExpressionLiteral.h"

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
		DebugPrint(" %s=%f", names[i], value);
	}
}
