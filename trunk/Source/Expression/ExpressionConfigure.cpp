#include "stdafx.h"

#include "ExpressionConfigure.h"

// instantiate loader templates
template Expression::Loader<float>;
template Expression::Loader<__m128>;

//
// configure an expression
template <typename T> void Expression::Loader<T>::Configure(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s expression %s\n", Expression::Schema<T>::NAME, element->Value());
#endif

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// get hash of tag name
	unsigned int hash = Hash(element->Value());

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == tinyxml2::XML_SUCCESS)
			overrided = true;
	}

	// if the tag matches a configure database entry...
	const Entry &entry = Get(hash);
	if (entry)
	{
		// use the entry
		entry(element, buffer, names, data);
		return;
	}

	// default to tag variable
	ConfigureTagVariable<T>(element, buffer, names, data);
}

// configure an expression root (the tag hosting the expression)
template <typename T> void Expression::Loader<T>::ConfigureRoot(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s root %s\n", Expression::Schema<T>::NAME, element->Value());
#endif

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == tinyxml2::XML_SUCCESS)
			overrided = true;
	}

	// special case: attribute variable reference
	if (element->Attribute("variable"))
	{
		ConfigureInlineVariable<T>(element, buffer, names, data);
		return;
	}

	// special case: attribute random value
	if (element->Attribute("rand"))
	{
		ConfigureRandom<T>(element, buffer, names, data);
		return;
	}

	// special case: embedded interpolator keyframes
	if (element->FirstChildElement("key"))
	{
		ConfigureInterpolator<T>(element, buffer, names, data);
		return;
	}

	// special case: no child elements
	if (!element->FirstChildElement())
	{
		// push literal data
		ConfigureLiteral<T>(element, buffer, names, data);
		return;
	}

	// special case: component elements
	for (int i = 0; i < width; ++i)
	{
		if (element->FirstChildElement(names[i]))
		{
			ConfigureConstruct<T>(element, buffer, names, data);
			return;
		}
	}

	// for each child node...
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		// recurse on child
		Configure(child, buffer, names, data);
	}
}

// specialization for boolean
template<> void Expression::Loader<bool>::Configure(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	unsigned int hash = Hash(element->Value());

	const Expression::Entry &entry = Expression::Loader<bool>::Get(hash);
	if (entry)
	{
		entry(element, buffer, sScalarNames, sScalarDefault);
		return;
	}

	Expression::Convert<bool, float>::Append(buffer);
	Expression::Loader<float>::Configure(element, buffer, sScalarNames, sScalarDefault);
}
