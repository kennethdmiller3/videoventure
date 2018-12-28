#include "StdAfx.h"

#include "Title.h"


// convert HSV [0..1] to RGB [0..1]
#pragma optimize( "t", on )
static void HSV2RGB(const float h, const float s, const float v, float &r, float &g, float &b)
{
#if 1
	// convert hue to index and fraction
	const int bits = 20;
	const int scaled = (xs_FloorToInt(h * (1 << bits)) & ((1 << bits) - 1)) * 6;
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
	}
#else
	// http://www.xmission.com/~trevin/atari/video_notes.html
	const float Y = 0.7f, S = 0.7f, theta = float(M_PI) - float(M_PI) * (sim_turn & 63) / 32.0f;
	float R = Clamp(Y + S * sin(theta), 0.0f, 1.0f);
	float G = Clamp(Y - (27/53) * S * sin(theta) - (10/53) * S * cos(theta), 0.0f, 1.0f);
	float B = Clamp(Y + S * cos(theta), 0.0f, 1.0f);
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

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
// mirror offset
static const float mirrorscale = -0.75f;

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
, titlebar(0)
{
}

ShellTitleTemplate::~ShellTitleTemplate()
{
	if (glIsList(titlebar))
		glDeleteLists(1, titlebar);
#if defined(USE_TITLE_DYNAMIC_TEXTURE)
	if (glIsList(titledrawlist))
		glDeleteLists(1, titledrawlist);
	if (glIsTexture(titletexture))
		glDeleteTextures(1, &titletexture);
#endif
	delete[] titlefill;
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

	// title bar drawlist
	titlebar = glGenLists(1);

	// begin drawlist
	glNewList(titlebar, GL_COMPILE);
	glBegin(GL_QUADS);

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
				if (alpha > 0.0f)
				{
					const float y0 = titley + row * titleh, y1 = y0 + titleh;

					glColor4f(0.3f, 0.3f, 0.3f, alpha);
					glVertex2f(0, y0);
					glVertex2f(640, y0);
					glVertex2f(640, y1);
					glVertex2f(0, y1);
				}

				++row;
			}
		}
	}

	// finish drawlist
	glEnd();
	glEndList();

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

			if (fill & (1<<4))
				fill = (1<<4);

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
	, titlebar(aTemplate.titlebar)
{
	SetAction(Action(this, &ShellTitle::Render));
}

ShellTitle::~ShellTitle()
{
}

// draw title
void ShellTitle::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
//#define USE_TITLE_VERTEX_ARRAY
#if defined(USE_TITLE_VERTEX_ARRAY)
	static Vector2 vertexarray[32768];
	static unsigned int colorarray[32768];
	Vector2 *vertexptr = vertexarray;
	unsigned int *colorptr = colorarray;
#endif

	// draw title bar
	glCallList(titlebar);

	// draw title body
	unsigned short *titlefillptr = titlefill;

#if !defined(USE_TITLE_DYNAMIC_TEXTURE)
#if !defined(USE_TITLE_VERTEX_ARRAY)
	glBegin(GL_QUADS);
#endif

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
	// mirror offset
	const float titleheight = titleh * (rows + 1);
	const float mirrortop = titley + titleheight + titleh * 2 + 4;
	const float mirrorbottom = mirrortop - mirrorscale * titleheight;
	const float mirroralphadelta = -0.375f / 32;
	const float mirroralphastart = 0.375f - mirroralphadelta * mirrortop;

	// starting mirror properties
	float mirror_y0 = mirrorbottom + MirrorWaveY(titley - titleh);
	float mirror_d0 = MirrorWaveX(mirror_y0);
	float mirror_a0 = mirroralphastart + mirroralphadelta * mirror_y0;
#endif

	for (int row = -1; row < rows + 1; ++row)
	{
		float y = titley + row * titleh;

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
		// row mirror properties
		float mirror_y1 = mirrorbottom + MirrorWaveY(y + titleh);
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
				bool border = (fill & ~(1<<4)) != 0;
				HSV2RGB(h + phase * 0.5f + border * 0.5f, 1.0f, 1.0f - 0.25f * border, R, G, B);

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
#ifdef USE_TITLE_VERTEX_ARRAY
						unsigned int color = 0xFF000000 | (xs_RoundToInt(B * 255) << 16) | (xs_RoundToInt(G * 255) << 8) | (xs_RoundToInt(R * 255) );
						*colorptr++ = color;
						*colorptr++ = color;
						*colorptr++ = color;
						*colorptr++ = color;
						*vertexptr++ = Vector2(x0, y0);
						*vertexptr++ = Vector2(x1, y0);
						*vertexptr++ = Vector2(x1, y1);
						*vertexptr++ = Vector2(x0, y1);
#else
						glColor4f(R, G, B, 1.0f);
						glVertex2f(x0, y0);
						glVertex2f(x1, y0);
						glVertex2f(x1, y1);
						glVertex2f(x0, y1);
#endif

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
						if (mirror_a0 > 0.0f || mirror_a1 > 0.0f)
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
#ifdef USE_TITLE_VERTEX_ARRAY
							color &= 0x00FFFFFF;
							color |= xs_RoundToInt(a1 * a1 * 255) << 24;
							*colorptr++ = color;
							*colorptr++ = color;
							*vertexptr++ = Vector2(x0 + dx1, yy1);
							*vertexptr++ = Vector2(x1 + dx1, yy1);
							color &= 0x00FFFFFF;
							color |= xs_RoundToInt(a0 * a0 * 255) << 24;
							*colorptr++ = color;
							*colorptr++ = color;
							*vertexptr++ = Vector2(x1 + dx0, yy0);
							*vertexptr++ = Vector2(x0 + dx0, yy0);
#else
							glColor4f(R, G, B, a1 * a1);
							glVertex2f(x0 + dx1, yy1);
							glVertex2f(x1 + dx1, yy1);
							glColor4f(R, G, B, a0 * a0);
							glVertex2f(x1 + dx0, yy0);
							glVertex2f(x0 + dx0, yy0);
#endif
						}
#endif
					}
				}
			}

			++titlefillptr;
		}

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
		// mirror shift row
		mirror_y0 = mirror_y1;
		mirror_d0 = mirror_d1;
		mirror_a0 = mirror_a1;
#endif
	}

#ifdef USE_TITLE_VERTEX_ARRAY
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertexarray);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, colorarray);
	glDrawArrays(GL_QUADS, 0, vertexptr - vertexarray);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
#else
	glEnd();
#endif

#else
	// texture-based variant

	// bind title texture
	glBindTexture(GL_TEXTURE_2D, titletexture);
	{int err=glGetError();if(err)DebugPrint("glBindTexture() error: %i\n",err);}
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// generate texture data
	unsigned char texturedata[titletexheight][titletexwidth][3];
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
				bool border = (fill & ~(1<<4)) != 0;
				HSV2RGB(h + phase * 0.5f + border * 0.5f, 1.0f, 1.0f - 0.25f * border, R, G, B);

				texturedata[row+1][col+1][0] = (unsigned char)(int)(R * 255);
				texturedata[row+1][col+1][1] = (unsigned char)(int)(G * 255);
				texturedata[row+1][col+1][2] = (unsigned char)(int)(B * 255);
			}
			else
			{
				texturedata[row+1][col+1][0] = 0;
				texturedata[row+1][col+1][1] = 0;
				texturedata[row+1][col+1][2] = 0;
			}

			++titlefillptr;
		}
	}

	// upload texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, titletexwidth, titletexheight, 0, GL_RGB, GL_UNSIGNED_BYTE, texturedata);
	{int err=glGetError();if(err)DebugPrint("glTexImage2D() error: %i\n",err);}

	// execute the drawlist
	glCallList(titledrawlist);
#endif
}
