#pragma once

extern void InitDrawlists(void);
extern void PreResetDrawlists(void);
extern void PostResetDrawlists(void);
extern void CleanupDrawlists(void);

struct BakeContext;
extern void ConfigureDrawItem(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake, BakeContext *context = NULL);
extern void ConfigureDrawItems(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, bool bake, BakeContext *context = NULL);

extern void RenderDynamicDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform);
extern void RenderStaticDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform);

struct EntityContext;
extern void RenderDrawlist(EntityContext &aContext, const Transform2 &aTransform);

// drawlist operations
extern void DO_AttribValue(EntityContext &aContext);
extern void DO_DrawMode(EntityContext &aContext);
extern void DO_BindTexture(EntityContext &aContext);
extern void DO_BlendFunc(EntityContext &aContext);
extern void DO_Disable(EntityContext &aContext);
extern void DO_CopyElements(EntityContext &aContext);
extern void DO_DrawElements(EntityContext &aContext);
extern void DO_Enable(EntityContext &aContext);
extern void DO_IndexPoints(EntityContext &aContext);
extern void DO_IndexLines(EntityContext &aContext);
extern void DO_IndexLineLoop(EntityContext &aContext);
extern void DO_IndexLineStrip(EntityContext &aContext);
extern void DO_IndexTriangles(EntityContext &aContext);
extern void DO_IndexTriangleStrip(EntityContext &aContext);
extern void DO_IndexTriangleFan(EntityContext &aContext);
extern void DO_IndexQuads(EntityContext &aContext);
extern void DO_IndexQuadStrip(EntityContext &aContext);
extern void DO_IndexPolygon(EntityContext &aContext);
extern void DO_LineWidth(EntityContext &aContext);
extern void DO_LineWidthWorld(EntityContext &aContext);
extern void DO_LoadIdentity(EntityContext &aContext);
extern void DO_LoadMatrix(EntityContext &aContext);
extern void DO_MultMatrix(EntityContext &aContext);
extern void DO_Normal(EntityContext &aContext);
extern void DO_PointSize(EntityContext &aContext);
extern void DO_PointSizeWorld(EntityContext &aContext);
extern void DO_PopAttrib(EntityContext &aContext);
extern void DO_PopMatrix(EntityContext &aContext);
extern void DO_PushAttrib(EntityContext &aContext);
extern void DO_PushMatrix(EntityContext &aContext);
extern void DO_Rotate(EntityContext &aContext);
extern void DO_Scale(EntityContext &aContext);
extern void DO_TexCoord(EntityContext &aContext);
extern void DO_TexEnvi(EntityContext &aContext);
extern void DO_Translate(EntityContext &aContext);
extern void DO_Vertex(EntityContext &aContext);
extern void DO_Repeat(EntityContext &aContext);
extern void DO_Block(EntityContext &aContext);
extern void DO_Swizzle(EntityContext &aContext);
extern void DO_Clear(EntityContext &aContext);
extern void DO_Loop(EntityContext &aContext);

namespace Database
{
	extern Typed<std::vector<unsigned int> > dynamicdrawlist;
	extern Typed<std::vector<unsigned int> > drawlist;
}
