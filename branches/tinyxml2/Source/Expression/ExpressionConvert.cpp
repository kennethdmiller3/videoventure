#include "stdafx.h"

#include "ExpressionConvert.h"

// configure conversion
template <typename T, typename A> void ConfigureConvert(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s convert %s\n", Expression::Schema<T>::NAME, Expression::Schema<A>::NAME);
#endif

	const tinyxml2::XMLElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for type conversion");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Convert<T, A>::Append(buffer);

	// append first argument
	Expression::Loader<A>::Configure(arg1, buffer, names, defaults);
}
