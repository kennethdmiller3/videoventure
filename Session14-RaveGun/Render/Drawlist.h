#pragma once

extern void ConfigureDrawItem(const TiXmlElement *element, std::vector<unsigned int> &buffer);
extern void ConfigureDrawItems(const TiXmlElement *element, std::vector<unsigned int> &buffer);
extern void RebuildDrawlists(void);
extern void RenderDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform);

namespace Database
{
	extern Typed<std::vector<unsigned int> > dynamicdrawlist;
	extern Typed<GLuint> drawlist;
	extern Typed<Typed<float> > variable;
}
