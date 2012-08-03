#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionEntity.h"


//
// VARIABLE EXPRESSION
// returns the value of a named variable
//

// evaluate typed variable
template <typename T> T EvaluateVariable(EntityContext &aContext);

// typed variable: attribute-inlined version
template <typename T> void ConfigureInlineVariable(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s variable %s (inline)\n", Expression::Schema<T>::NAME, element->Attribute("variable"));
#endif
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("variable")));
}

// typed variable: normal version
template <typename T> void ConfigureVariable(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s variable %s\n", Expression::Schema<T>::NAME, element->Attribute("name"));
#endif
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("name")));
}

// typed variable: tag-named version
template <typename T> void ConfigureTagVariable(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s variable %s (tag)\n", Expression::Schema<T>::NAME, element->Value());
#endif
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Value()));
}
