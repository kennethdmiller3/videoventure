#pragma once

extern void InitDrawlists(void);
extern void CleanupDrawlists(void);

extern void ConfigureDrawItem(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake);
extern void ConfigureDrawItems(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake);

extern void RebuildDrawlists(void);

extern void RenderDynamicDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform);
extern void RenderStaticDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform);

struct EntityContext;
extern void RenderDrawlist(EntityContext &aContext, const Transform2 &aTransform);

extern void RenderFlush();

namespace Database
{
	extern Typed<std::vector<unsigned int> > dynamicdrawlist;
	extern Typed<std::vector<unsigned int> > drawlist;
}
