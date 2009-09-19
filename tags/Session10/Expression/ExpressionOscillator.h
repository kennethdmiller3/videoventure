#pragma once

void ConfigureSineWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
void ConfigureTriangleWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
void ConfigureSawtoothWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
void ConfigurePulseWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
