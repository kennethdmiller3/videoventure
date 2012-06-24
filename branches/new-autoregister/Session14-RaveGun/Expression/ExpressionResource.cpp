#include "StdAfx.h"

#include "ExpressionSchema.h"
#include "ExpressionResource.h"
#include "Resource.h"


//
// RESOURCE EXPRESSION
// returns the value of a named resource
//

// evaluate resource
float EvaluateResource(EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	unsigned int id = FindResource(aContext.mId, name);
	const Resource *resource = Database::resource.Get(id).Get(name);
	return resource ? resource->GetValue() : 0.0f;
}

// typed resource: attribute-inlined version
void ConfigureInlineResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a resource expression
	DebugPrint("%s resource %s (inline)\n", Expression::Schema<float>::NAME, element->Attribute("resource"));
	Expression::Append(buffer, EvaluateResource, Hash(element->Attribute("resource")));
}

// typed resource: normal version
void ConfigureResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a resource expression
	DebugPrint("%s resource %s\n", Expression::Schema<float>::NAME, element->Attribute("name"));
	Expression::Append(buffer, EvaluateResource, Hash(element->Attribute("name")));
}
