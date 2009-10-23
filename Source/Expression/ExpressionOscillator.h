#pragma once

GAME_API void ConfigureSineWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
GAME_API void ConfigureTriangleWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
GAME_API void ConfigureSawtoothWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
GAME_API void ConfigurePulseWave(const TiXmlElement *element, std::vector<unsigned int> &buffer);
