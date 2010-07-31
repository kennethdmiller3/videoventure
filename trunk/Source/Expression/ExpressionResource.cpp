#include "StdAfx.h"

#include "ExpressionSchema.h"
#include "ExpressionResource.h"
#include "ExpressionConfigure.h"
#include "ExpressionConvert.h"
#include "Resource.h"

template<typename T> static void ConfigureResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	Expression::Convert<T, float>::Append(buffer);
	::ConfigureResource(element, buffer, sScalarNames, sScalarDefault);
}

static ExpressionConfigure::Auto<float> resourcefloat(0x29df7ff5 /* "resource" */, ConfigureResource<float>);
static ExpressionConfigure::Auto<__m128> resourcevector(0x29df7ff5 /* "resource" */, ConfigureResource<__m128>);

//
// RESOURCE EXPRESSION
// returns the value of a named resource
//

// evaluate resource
float EvaluateResource(EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	unsigned int id = FindResourceContainer(aContext.mId, name);
	const Resource *resource = Database::resource.Get(id).Get(name);
	return resource ? resource->GetValue() : 0.0f;
}

// typed resource: attribute-inlined version
void ConfigureInlineResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a resource expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s resource %s (inline)\n", Expression::Schema<float>::NAME, element->Attribute("resource"));
#endif
	Expression::Append(buffer, EvaluateResource, Hash(element->Attribute("resource")));
}

// typed resource: normal version
void ConfigureResource(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a resource expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s resource %s\n", Expression::Schema<float>::NAME, element->Attribute("name"));
#endif
	Expression::Append(buffer, EvaluateResource, Hash(element->Attribute("name")));
}
