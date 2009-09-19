#pragma once

struct EntityContext;

namespace Expression
{
	void Spawn(EntityContext &aContext);
	void Switch(EntityContext &aContext);
	void AddResource(EntityContext &aContext);
	void Repeat(EntityContext &aContext);
	void Loop(EntityContext &aContext);
}

void ConfigureAction(const TiXmlElement *element, std::vector<unsigned int> &buffer);
