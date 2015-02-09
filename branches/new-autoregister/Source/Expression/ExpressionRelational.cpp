#include "stdafx.h"

#include "ExpressionRelational.h"
#include "ExpressionConfigure.h"

static void ConfigureGreater(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::Greater<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool> greaterbool(0x50c80b99 /* "greater" */, ConfigureGreater);

static void ConfigureGreaterEqual(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::GreaterEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool> greaterequalbool(0xf75208d3 /* "greaterequal" */, ConfigureGreaterEqual);

static void ConfigureLess(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::Less<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool> lessbool(0x216b57b8 /* "less" */, ConfigureLess);

static void ConfigureLessEqual(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::LessEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool> lessequalbool(0xce1f56b0 /* "lessequal" */, ConfigureLessEqual);

static void ConfigureEqual(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::Equal<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool> equalbool(0x2f7508ef /* "equal" */, ConfigureEqual);

static void ConfigureNotEqual(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	ConfigureBinary<bool, float, float>(Expression::NotEqual<float>, element, buffer, sScalarNames, sScalarDefault);
}
static Expression::Loader<bool> notequalbool(0x7eca4a1e /* "notequal" */, ConfigureNotEqual);