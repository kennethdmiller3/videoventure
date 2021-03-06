#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"

// float[width] literal
GAME_API void ConfigureLiteral(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[]);

// typed literal
template <typename T> void ConfigureLiteral(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a constant expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s literal:", Expression::Schema<T>::NAME);
#endif
	Expression::Append(buffer, Expression::Read<T>);
	ConfigureLiteral(element, buffer, sizeof(T)/sizeof(float), names, defaults);
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("\n");
#endif
}
