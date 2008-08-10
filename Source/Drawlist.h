#pragma once

extern void ProcessDrawItem(const TiXmlElement *element, std::vector<unsigned int> &buffer);
extern void ProcessDrawItems(const TiXmlElement *element, std::vector<unsigned int> &buffer);
extern void ExecuteDrawItems(const unsigned int buffer[], size_t count, float param, unsigned int id);
extern void RebuildDrawlists(void);

namespace Database
{
	extern Typed<std::vector<unsigned int> > dynamicdrawlist;
	extern Typed<GLuint> drawlist;
	extern Typed<GLuint> texture;
	extern Typed<Typed<float> > variable;
}
