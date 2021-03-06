#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionEntity.h"


//
// VARIABLE EXPRESSION
// returns the value of a named variable
//

// evaluate float[width] variable
inline void EvaluateVariable(float value[], int width, EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
//	const Database::Typed<float> &variables = Database::variable.Get(aContext.mId);
	for (int i = 0; i < width; ++i)
		value[i] = aContext.mVars->Get(name+i);
}

// evaluate typed variable
template <typename T> static const T EvaluateVariable(EntityContext &aContext)
{
	T value = T();
	EvaluateVariable(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), aContext);
	return value;
}

// typed variable: attribute-inlined version
template <typename T> void ConfigureInlineVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	DebugPrint("%s variable %s (inline)\n", Expression::Schema<T>::NAME, element->Attribute("variable"));
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("variable")));
}

// typed variable: normal version
template <typename T> void ConfigureVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	DebugPrint("%s variable %s\n", Expression::Schema<T>::NAME, element->Attribute("name"));
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("name")));
}

// typed variable: tag-named version
template <typename T> void ConfigureTagVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	DebugPrint("%s variable %s (tag)\n", Expression::Schema<T>::NAME, element->Value());
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Value()));
}
