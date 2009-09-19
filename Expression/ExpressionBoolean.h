#pragma once

template <typename T> void ConfigureExpression(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);

template<> void ConfigureExpression<bool>(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
