#pragma once

struct EntityContext;

namespace Expression
{
	void GAME_API Spawn(EntityContext &aContext);
	void GAME_API Switch(EntityContext &aContext);
	void GAME_API AddResource(EntityContext &aContext);
	void GAME_API Repeat(EntityContext &aContext);
	void GAME_API Loop(EntityContext &aContext);
}

void GAME_API ConfigureAction(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
