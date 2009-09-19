#pragma once

#include "Expression.h"
#include "ExpressionEntity.h"


//
// RESOURCE EXPRESSION
// returns the value of a named resource
//

// evaluate resource
float EvaluateResource(EntityContext &aContext);

// typed resource: attribute-inlined version
void ConfigureInlineResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);

// typed resource: normal version
void ConfigureResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
