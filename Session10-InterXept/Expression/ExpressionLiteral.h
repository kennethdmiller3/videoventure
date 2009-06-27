#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"

// float[width] literal
void ConfigureLiteral(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[]);

// typed literal
template <typename T> void ConfigureLiteral(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a constant expression
	DebugPrint("%s literal:", Expression::Schema<T>::NAME);
	Expression::Append(buffer, Expression::Constant<T>);
	ConfigureLiteral(element, buffer, sizeof(T)/sizeof(float), names, defaults);
	DebugPrint("\n");
}
