#pragma once

extern void InitDrawlists(void);
extern void CleanupDrawlists(void);

extern void ConfigureDrawItem(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
extern void ConfigureDrawItems(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer);
extern void RebuildDrawlists(void);
extern void RenderDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform);

namespace Database
{
	extern Typed<std::vector<unsigned int> > dynamicdrawlist;
	extern Typed<GLuint> drawlist;
}
