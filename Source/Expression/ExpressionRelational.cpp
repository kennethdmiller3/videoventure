#include "stdafx.h"

#include "ExpressionRelational.h"
#include "ExpressionConfigure.h"

static void ConfigureGreater(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::Greater<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool>::Auto greaterbool(0x50c80b99 /* "greater" */, ConfigureGreater);

static void ConfigureGreaterEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::GreaterEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool>::Auto greaterequalbool(0xf75208d3 /* "greaterequal" */, ConfigureGreaterEqual);

static void ConfigureLess(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::Less<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool>::Auto lessbool(0x216b57b8 /* "less" */, ConfigureLess);

static void ConfigureLessEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::LessEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool>::Auto lessequalbool(0xce1f56b0 /* "lessequal" */, ConfigureLessEqual);

static void ConfigureEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::Equal<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool>::Auto equalbool(0x2f7508ef /* "equal" */, ConfigureEqual);

static void ConfigureNotEqual(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::NotEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool>::Auto notequalbool(0x7eca4a1e /* "notequal" */, ConfigureNotEqual);