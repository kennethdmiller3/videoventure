#pragma once

extern void ConfigureDrawItem(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
extern void ConfigureDrawItems(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
extern void RebuildDrawlists(void);
extern void RenderDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform);

namespace Database
{
	extern Typed<std::vector<unsigned int> > dynamicdrawlist;
	extern Typed<GLuint> drawlist;
}
