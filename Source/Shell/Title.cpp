#include "StdAfx.h"

#include "Title.h"
#include "Render.h"
#include "Magic.h"
#include "MatrixStack.h"
#include "ShaderColor.h"

#define USE_TITLE_PACKED_VERTEX
#if defined(USE_TITLE_PACKED_VERTEX)
struct Vertex
{
	Vector2 pos;
	unsigned int color;
};
#else
struct Vertex
{
	Vector3 pos;
	Color4 color;
};
#endif

// convert HSV [0..1] to RGB [0..1]
#pragma optimize( "t", on )
static void HSV2RGB(const float h, const float s, const float v, float &r, float &g, float &b)
{
#if 1
	// convert hue to index and fraction
	const int bits = 20;
	const int scaled = (FloorToInt(h * (1 << bits)) & ((1 << bits) - 1)) * 6;
	const int i = scaled >> bits;
	const float f = scaled * (1.0f / (1 << bits)) - i;

	// generate components
	const float p = v * (1 - s);
	const float q = v * (1 - f * s);
	const float t = v * (1 - (1 - f) * s);

	switch (i)
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	default: __assume(0);
	}
#else
	// http://www.xmission.com/~trevin/atari/video_notes.html
	const float Y = 0.7f * v, S = 0.7f * s, theta = 2.0f * float(M_PI) * h;
	const float R = Y + S * sinf(theta);
	const float B = Y + S * cosf(theta);
	const float G = Y - (27 / 53) * (R - Y) - (10 / 53) * (B - Y);
	r = Clamp(R, 0.0f, 1.0f);
	g = Clamp(G, 0.0f, 1.0f);
	b = Clamp(B, 0.0f, 1.0f);
#endif
}
#pragma optimize( "", on )

// border drawing properties
static const float borderw = 2;
static const float borderh = 2;

// title drawing properties
static const float titlew = 6;
static const float titleh = 6;
static const float titlex = 320;
static const float titley = 16;
static const float titlez = 0;

// border rectangles
enum BorderCorner
{
	BORDER_UL,
	BORDER_U,
	BORDER_UR,
	BORDER_L,
	BORDER_C,
	BORDER_R,
	BORDER_BL,
	BORDER_B,
	BORDER_BR
};
static const float block[9][2][2] =
{
	{ { 0, borderw }, { 0, borderh } },
	{ { borderw, titlew - borderw }, { 0, borderh } },
	{ { titlew - borderw, titlew }, { 0, borderh } },
	{ { 0, borderw }, { borderh, titleh - borderh } },
	{ { 0, titlew }, { 0, titleh } },	// <-- filled block
	{ { titlew - borderw, titlew }, { borderh, titleh - borderh } },
	{ { 0, borderw }, { titleh - borderh, titleh } },
	{ { borderw, titlew - borderh}, { titleh - borderh, titleh } },
	{ { titlew - borderw, titlew }, { titleh - borderh, titleh } },
};
static const int mask[9] =
{
	(1<<BORDER_UL), ((1<<BORDER_UL)|(1<<BORDER_U)|(1<<BORDER_UR)), (1<<BORDER_UR),
	((1<<BORDER_UL)|(1<<BORDER_L)|(1<<BORDER_BL)), (1 << BORDER_C), ((1<<BORDER_UR)|(1<<BORDER_R)|(1<<BORDER_BR)),
	(1<<BORDER_BL), ((1<<BORDER_BL)|(1<<BORDER_B)|(1<<BORDER_BR)), (1<<BORDER_BR)
};

//#define USE_TITLE_DYNAMIC_TEXTURE
#define USE_TITLE_MIRROR_WATER_EFFECT

#if defined(USE_TITLE_DYNAMIC_TEXTURE)

// constants
static const int titletexwidth = 128;
static const int titletexheight = 64;
static const float titleborderu = float(borderw) / float(titlew * titletexwidth);
static const float titleborderv = float(borderh) / float(titleh * titletexheight);

// title texture handle
static GLuint titletexture;

// title drawlist handle
static GLuint titledrawlist;

#endif

#if defined(USE_TITLE_MIRROR_WATER_EFFECT)
// mirror effect properties
static const float mirrorscale = -0.75f;
static const float mirroralpha = 0.5f;
static const float mirrorheight = 48.0f;
static const int mirrorrows = int(ceilf(mirrorheight / (titleh * -mirrorscale)));

// mirror y-axis wave function
static float MirrorWaveY(float y)
{
	return mirrorscale * y + 1.0f * sinf(sim_turn / 64.0f + y / 8.0f) + 3.0f * sinf(sim_turn / 128.0f + y / 32.0f);
}

// mirror x-axis wave function
static float MirrorWaveX(float y)
{
	return 1.0f * sinf(sim_turn / 32.0f + y / 4.0f);
}
#endif

// block color
static float BlockHue(int col, int row)
{
	return sim_turn / 1024.0f + row / 128.0f + 0.03125f * sinf(sim_turn / 64.0f + row / 4.0f + 4.0f * sinf(sim_turn / 64.0f + col / 8.0f + 0.5f * sinf(sim_turn / 64.0f + row / 4.0f)));
}

namespace Database
{
	Typed<ShellTitleTemplate> shelltitletemplate(0xbc5f3ad3 /* "shelltitletemplate" */);
	Typed<ShellTitle *> shelltitle(0x45e6e74d /* "shelltitle" */);

	namespace Loader
	{
		static void ShellTitleConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			ShellTitleTemplate &shelltitle = Database::shelltitletemplate.Open(aId);
			shelltitle.Configure(element, aId);
			Database::shelltitletemplate.Close(aId);
		}
		Configure shelltitleconfigure(0x45e6e74d /* "shelltitle" */, ShellTitleConfigure);
	}

	namespace Initializer
	{
		static void ShellTitleActivate(unsigned int aId)
		{
			const ShellTitleTemplate &shelltitletemplate = Database::shelltitletemplate.Get(aId);
			ShellTitle *shelltitle = new ShellTitle(shelltitletemplate, aId);
			Database::shelltitle.Put(aId, shelltitle);
			shelltitle->Show();
		}
		Activate shelltitleactivate(0xbc5f3ad3 /* "shelltitletemplate" */, ShellTitleActivate);

		static void ShellTitleDeactivate(unsigned int aId)
		{
			if (ShellTitle *shelltitle = Database::shelltitle.Get(aId))
			{
				shelltitle->Hide();
				delete shelltitle;
				Database::shelltitle.Delete(aId);
			}
		}
		Deactivate shelltitledeactivate(0xbc5f3ad3 /* "shelltitletemplate" */, ShellTitleDeactivate);
	}
}

ShellTitleTemplate::ShellTitleTemplate(void)
: OverlayTemplate()
, cols(0)
, rows(0)
, titlefill(NULL)
, rowalpha(NULL)
, vertcount(0)
{
}

ShellTitleTemplate::~ShellTitleTemplate()
{
#if defined(USE_TITLE_DYNAMIC_TEXTURE)
	if (glIsList(titledrawlist))
		glDeleteLists(1, titledrawlist);
	if (glIsTexture(titletexture))
		glDeleteTextures(1, &titletexture);
#endif
	delete[] titlefill;
	delete[] rowalpha;
}

// configure
bool ShellTitleTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	OverlayTemplate::Configure(element, aId);

	// get column and row count
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x440e1d7b /* "row" */:
			{
				const char *text = child->Attribute("data");
				if (!text)
					text = child->GetText();
				if (!text)
					continue;
				++rows;
				cols = std::max<int>(cols, int(strlen(text)));
			}
			break;
		}
	}

	// temporary data
	char *titlemap = (char *)_alloca(cols * rows);
	memset(titlemap, ' ', cols * rows);

	// allocate row alphas
	rowalpha = new float[rows];

	// fill in
	int row = 0;
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x440e1d7b /* "row" */:
			{
				const char *text = child->Attribute("data");
				if (!text)
					text = child->GetText();
				if (!text)
					continue;
				memcpy(&titlemap[row*cols], text, strlen(text));

				float alpha = 0.0f;
				child->QueryFloatAttribute("bar", &alpha);

				if (alpha != 0.0f)
					vertcount += 4;

				rowalpha[row] = alpha;

				++row;
			}
		}
	}

	// allocate fill data
	titlefill = new unsigned short[(rows + 2) * (cols + 2)];

	// generate fill data
	unsigned short *titlefillptr = titlefill;
	for (int row = -1; row < rows + 1; ++row)
	{
		for (int col = -1; col < cols + 1; ++col)
		{
			int phase = 0;
			int fill = 0;

			int c0 = std::max<int>(col - 1, 0);
			int c1 = std::min<int>(col + 1, cols - 1);
			int r0 = std::max<int>(row - 1, 0);
			int r1 = std::min<int>(row + 1, rows - 1);

			for (int r = r0; r <= r1; ++r)
			{
				for (int c = c0; c <= c1; ++c)
				{
					if (titlemap[r*cols+c] >= '0')
					{
						phase = titlemap[r*cols+c] - '0';
						fill |= mask[(r - row + 1) * 3 + (c - col + 1)];
					}
				}
			}

			if (fill & (1<<BORDER_C))
				fill = (1<<BORDER_C);

			if (fill)
			{
				int quads = count_ones(fill) * 4;
				vertcount += quads;
				if (row > rows - mirrorrows)
					vertcount += quads;
			}

			*titlefillptr++ = unsigned short(fill | (phase << 9));
		}
	}

#if defined(USE_TITLE_DYNAMIC_TEXTURE)

	// generate texture handle
	glGenTextures(1, &titletexture);
	{int err=glGetError();if(err)DebugPrint("glGenTextures() error: %i\n",err);}

	// generate drawlist handle
	titledrawlist = glGenLists(1);
	{int err=glGetError();if(err)DebugPrint("glGenLists() error: %i\n",err);}

	// initialize slab map
	unsigned char *titleslabmap = (unsigned char *)_alloca(rows*cols);
	memset(titleslabmap, 0xFF, rows*cols);
	int titleslabcount = 0;

	// begin drawlist
	glNewList(titledrawlist, GL_COMPILE);

	// enable texture
	glPushAttrib(GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_2D);

	// reset color
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// begin primitive
	glBegin(GL_QUADS);

	// generate title slabs
	for (int row = 0; row < rows; ++row)
	{
		for (int col = 0; col < cols; ++col)
		{
			// skip empty spaces
			if (titlemap[row*cols+col] == ' ')
				continue;

			// skip assigned spaces
			if (titleslabmap[row*cols+col] != 0xFF)
				continue;

			// allocate a new index
			int index = titleslabcount++;

			// find horizontal extent
			int c0 = col;
			int c1 = cols;
			for (int c = c0; c < c1; ++c)
			{
				if ((titlemap[row*cols+c] == ' ') || ((titleslabmap[row*cols+c] != 0xFF) && (titleslabmap[row*cols+c] != index)))
				{
					c1 = c;
					break;
				}
			}

			// find vertical extent
			int r0 = row;
			int r1 = rows;
			for (int r = r0; r < r1; ++r)
			{
				for (int c = c0; c < c1; ++c)
				{
					if ((titlemap[r*cols+c] == ' ') || ((titleslabmap[r*cols+c] != 0xFF) && (titleslabmap[r*cols+c] != index)))
					{
						r1 = r;
						break;
					}
				}
			}

			// fill slab
			for (int r = r0; r < r1; ++r)
			{
				for (int c = c0; c < c1; ++c)
				{
					titleslabmap[r*cols+c] = (unsigned char)index;
				}
			}

			assert(c0 < c1 && r0 < r1);

			// generate texture extents
			float u0 = float(c0+1) / titletexwidth - titleborderu, u1 = float(c1+1) / titletexwidth + titleborderu;
			float v0 = float(r0+1) / titletexheight - titleborderv, v1 = float(r1+1) / titletexheight + titleborderv;

			// generate position extents
			float x0 = titlex + (c0 - cols * 0.5f) * titlew - borderw, x1 = titlex + (c1 - cols * 0.5f) * titlew + borderw;
			float y0 = titley + r0 * titleh - borderh, y1 = titley + r1 * titleh + borderh;

			// submit vertices
			glTexCoord2f(u0, v0); glVertex2f(x0, y0);
			glTexCoord2f(u1, v0); glVertex2f(x1, y0);
			glTexCoord2f(u1, v1); glVertex2f(x1, y1);
			glTexCoord2f(u0, v1); glVertex2f(x0, y1);

			// skip visited columns
			col = c1;
		}
	}

	glEnd();
	glPopAttrib();

	glEndList();

#endif

	return true;
}

ShellTitle::ShellTitle(const ShellTitleTemplate &aTemplate, unsigned int aId)
	: Overlay(aId)
	, cols(aTemplate.cols)
	, rows(aTemplate.rows)
	, titlefill(aTemplate.titlefill)
	, rowalpha(aTemplate.rowalpha)
	, vertcount(aTemplate.vertcount)
{
	SetAction(Action(this, &ShellTitle::Render));
}

ShellTitle::~ShellTitle()
{
}

// draw title
void ShellTitle::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// begin drawing

	// use the color shader
	if (UseProgram(ShaderColor::gProgramId) || &GetBoundVertexBuffer() != &GetDynamicVertexBuffer() || ViewProjChanged())
	{
		// changed program or switching back from non-dynamic geometry:
		// set model view projection matrix
		SetUniformMatrix4(ShaderColor::gUniformModelViewProj, ViewProjGet());
	}

	// set attribute formats
	SetAttribFormat(ShaderColor::gAttribPosition, 2, GL_FLOAT);
	SetAttribFormat(ShaderColor::gAttribColor, 4, GL_UNSIGNED_BYTE);

	// set work buffer format
	SetWorkFormat((1<<ShaderColor::gAttribPosition)|(1<<ShaderColor::gAttribColor));

	// draw triangle list 
	// (because quads are deprecated)
	SetDrawMode(GL_TRIANGLES);

	// allocate vertex data
	Vertex *v0 = static_cast<Vertex *>(AllocVertices(vertcount));
	register Vertex * __restrict v = v0;

	// draw title bar
	for (int row = 0; row < rows; ++row)
	{
		const float alpha = rowalpha[row];
		if (alpha > 0.0f)
		{
			const float y0 = titley + row * titleh, y1 = y0 + titleh;
#if defined(USE_TITLE_PACKED_VERTEX)
			const unsigned int color = 0x004C4C4C | (unsigned int(floorf(0.5f + alpha * 255)) << 24);
			v->pos = Vector2(0.0f, y0);
			v->color = color;
			++v;
			v->pos = Vector2(640.0f, y0);
			v->color = color;
			++v;
			v->pos = Vector2(640.0f, y1);
			v->color = color;
			++v;
			v->pos = Vector2(0.0f, y1);
			v->color = color;
			++v;
#else
			v->pos = Vector3(0.0f, y0, 0.0f);
			v->color = Color4(0.3f, 0.3f, 0.3f, alpha);
			++v;
			v->pos = Vector3(640.0f, y0, 0.0f);
			v->color = Color4(0.3f, 0.3f, 0.3f, alpha);
			++v;
			v->pos = Vector3(640.0f, y1, 0.0f);
			v->color = Color4(0.3f, 0.3f, 0.3f, alpha);
			++v;
			v->pos = Vector3(0.0f, y1, 0.0f);
			v->color = Color4(0.3f, 0.3f, 0.3f, alpha);
			++v;
#endif
		}
	}

	// draw title body
	unsigned short *titlefillptr = titlefill;

#if !defined(USE_TITLE_DYNAMIC_TEXTURE)

#if defined(USE_TITLE_MIRROR_WATER_EFFECT)
	// mirror offset
	const float titleheight = titleh * (rows + 1);
	const float mirrortop = titley + titleheight + 4;
	const float mirrorbottom = mirrortop - mirrorscale * titleheight;
	const float mirroralphadelta = -mirroralpha / mirrorheight;
	const float mirroralphastart = mirroralpha - mirroralphadelta * mirrortop;

	// starting mirror properties
	float mirror_y0 = mirrorbottom + MirrorWaveY(-titleh);
	float mirror_d0 = MirrorWaveX(mirror_y0);
	float mirror_a0 = mirroralphastart + mirroralphadelta * mirror_y0;
#endif

	for (int row = -1; row < rows + 1; ++row)
	{
		float y = titley + row * titleh;

#if defined(USE_TITLE_MIRROR_WATER_EFFECT)
		// row mirror properties
		float mirror_y1 = mirrorbottom + MirrorWaveY((row + 1) * titleh);
		float mirror_yd = (mirror_y1 - mirror_y0) / titleh;
		float mirror_d1 = MirrorWaveX(mirror_y1);
		float mirror_dd = (mirror_d1 - mirror_d0) / titleh;
		float mirror_a1 = mirroralphastart + mirroralphadelta * mirror_y1;
		float mirror_ad = (mirror_a1 - mirror_a0) / titleh;
#endif

		for (int col = -1; col < cols + 1; ++col)
		{
			float x = titlex + (col - 0.5f * cols) * titlew;

			if (*titlefillptr != 0)
			{
				int phase = *titlefillptr >> 9;
				int fill = *titlefillptr & 0x1FF;

				// get block color
				float R, G, B;
				float h = BlockHue(col, row);
				bool border = (fill & ~(1<<BORDER_C)) != 0;
				HSV2RGB(h + phase * 0.5f + border * 0.5f, 1.0f, 1.0f - 0.25f * border, R, G, B);
				unsigned int color = 0xFF000000 
					| (RoundToInt(B * 255) << 16) 
					| (RoundToInt(G * 255) << 8)
					| (RoundToInt(R * 255));

				// for each block...
				for (int i = 0; i < 9; ++i)
				{
					// if the block is filled
					if (fill & (1 << i))
					{
						// block borders
						float x0 = x + block[i][0][0];
						float x1 = x + block[i][0][1];
						float y0 = y + block[i][1][0];
						float y1 = y + block[i][1][1];

						// upright
#if defined(USE_TITLE_PACKED_VERTEX)
						v->pos = Vector2(x0, y0);
						v->color = color;
						++v;
						v->pos = Vector2(x1, y0);
						v->color = color;
						++v;
						v->pos = Vector2(x1, y1);
						v->color = color;
						++v;
						v->pos = Vector2(x0, y1);
						v->color = color;
						++v;
#else
						v->pos = Vector3(x0, y0, 0);
						v->color = Color4(R, G, B, 1);
						++v;
						v->pos = Vector3(x1, y0, 0);
						v->color = Color4(R, G, B, 1);
						++v;
						v->pos = Vector3(x1, y1, 0);
						v->color = Color4(R, G, B, 1);
						++v;
						v->pos = Vector3(x0, y1, 0);
						v->color = Color4(R, G, B, 1);
						++v;
#endif

#if defined(USE_TITLE_MIRROR_WATER_EFFECT)
						if (row > rows - mirrorrows) //(mirror_a0 > 0.0f || mirror_a1 > 0.0f)
						{
							// mirrored
							float m0 = y0 - y;
							float m1 = y1 - y;
							float a0 = std::max(mirror_a0 + mirror_ad * m0, 0.0f);
							float a1 = std::max(mirror_a0 + mirror_ad * m1, 0.0f);
							float dx0 = mirror_d0 + mirror_dd * m0;
							float dx1 = mirror_d0 + mirror_dd * m1;
							float yy0 = mirror_y0 + mirror_yd * m0;
							float yy1 = mirror_y0 + mirror_yd * m1;
#if defined(USE_TITLE_PACKED_VERTEX)
							unsigned int color1 = (RoundToInt(a1 * a1 * 255) << 24) | (color & 0x00FFFFFF);
							v->pos = Vector2(x0 + dx1, yy1);
							v->color = color1;
							++v;
							v->pos = Vector2(x1 + dx1, yy1);
							v->color = color1;
							++v;
							unsigned int color0 = (RoundToInt(a0 * a0 * 255) << 24) | (color & 0x00FFFFFF);
							v->pos = Vector2(x1 + dx0, yy0);
							v->color = color0;
							++v;
							v->pos = Vector2(x0 + dx0, yy0);
							v->color = color0;
							++v;
#else
							v->pos = Vector3(x0 + dx1, yy1, 0);
							v->color = Color4(R, G, B, a1 * a1);
							++v;
							v->pos = Vector3(x1 + dx1, yy1, 0);
							v->color = Color4(R, G, B, a1 * a1);
							++v;
							v->pos = Vector3(x1 + dx0, yy0, 0);
							v->color = Color4(R, G, B, a0 * a0);
							++v;
							v->pos = Vector3(x0 + dx0, yy0, 0);
							v->color = Color4(R, G, B, a0 * a0);
							++v;
#endif
						}
#endif
					}
				}
			}

			++titlefillptr;
		}

#if defined(USE_TITLE_MIRROR_WATER_EFFECT)
		// mirror shift row
		mirror_y0 = mirror_y1;
		mirror_d0 = mirror_d1;
		mirror_a0 = mirror_a1;
#endif
	}

	// generate indices
	IndexQuads(GetVertexCount() - vertcount, vertcount);

	// finish drawing
	assert(v == v0 + vertcount);
	FlushDynamic();

#else
	// texture-based variant

	// bind title texture
	glBindTexture(GL_TEXTURE_2D, titletexture);
	{int err=glGetError();if(err)DebugPrint("glBindTexture() error: %i\n",err);}
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// execute the drawlist
	glCallList(titledrawlist);

	// generate texture data
	unsigned char texturedata[titletexheight][titletexwidth][4];
	for (int row = -1; row < rows + 1; ++row)
	{
		for (int col = -1; col < cols + 1; ++col)
		{
			if (*titlefillptr != 0)
			{
				int phase = *titlefillptr >> 9;
				int fill = *titlefillptr & 0x1FF;

				// get block color
				float R, G, B;
				float h = BlockHue(col, row);
				bool border = (fill & ~(1<<BORDER_C)) != 0;
				HSV2RGB(h + phase * 0.5f + border * 0.5f, 1.0f, 1.0f - 0.25f * border, R, G, B);

				texturedata[row+1][col+1][0] = (unsigned char)(int)(R * 255);
				texturedata[row+1][col+1][1] = (unsigned char)(int)(G * 255);
				texturedata[row+1][col+1][2] = (unsigned char)(int)(B * 255);
				texturedata[row+1][col+1][3] = 255;
			}
			else
			{
				texturedata[row+1][col+1][0] = 0;
				texturedata[row+1][col+1][1] = 0;
				texturedata[row+1][col+1][2] = 0;
				texturedata[row+1][col+1][3] = 0;
			}

			++titlefillptr;
		}
	}

	// upload texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, titletexwidth, titletexheight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, texturedata);
	{int err=glGetError();if(err)DebugPrint("glTexImage2D() error: %i\n",err);}

	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}
