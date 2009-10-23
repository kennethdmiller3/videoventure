#pragma once

#include "Expression.h"
#include "ExpressionEntity.h"


//
// RESOURCE EXPRESSION
// returns the value of a named resource
//

// evaluate resource
GAME_API float EvaluateResource(EntityContext &aContext);

// typed resource: attribute-inlined version
GAME_API void ConfigureInlineResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);

// typed resource: normal version
GAME_API void ConfigureResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
