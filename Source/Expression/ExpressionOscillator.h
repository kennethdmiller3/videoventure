#pragma once

GAME_API void ConfigureSineWave(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
GAME_API void ConfigureTriangleWave(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
GAME_API void ConfigureSawtoothWave(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
GAME_API void ConfigurePulseWave(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
