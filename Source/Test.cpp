// precompiled header
#include "stdafx.h"

// includes
#include "Timer.h"
#include "Cloud.h"
#include "Grid.h"
#include "Player.h"
#include "Gunner.h"
#include "Target.h"

// entity map
EntityMap entities;

// drawlist map
DrawListMap drawlists;

int DebugPrint(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
#ifdef WIN32
	char buf[4096];
	int n = vsnprintf(buf, sizeof(buf), format, ap);
	OutputDebugStringA(buf);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

bool init_GL()
{	
	// set clear color
	glClearColor( 0, 0, 0, 0 );

#ifdef ENABLE_ANTIALIAS_POINT
	// enable point smoothing
	glEnable( GL_POINT_SMOOTH );
	glHint( GL_POINT_SMOOTH_HINT, GL_DONT_CARE );
#endif
#ifdef ENABLE_ANTIALIAS_LINE
	// enable line smoothing
 	glEnable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_DONT_CARE );
#endif
#ifdef ENABLE_ANTIALIAS_POLYGON
	// enable polygon smoothing
	glEnable( GL_POLYGON_SMOOTH );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE );
#endif

	// enable blending
	glEnable( GL_BLEND );
#ifdef ENABLE_SRC_ALPHA_SATURATE
	glBlendFunc( GL_SRC_ALPHA_SATURATE, GL_ONE );
#else
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
#endif

#ifdef ENABLE_DEPTH_TEST
	// enable z test
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
#else
	glDisable( GL_DEPTH_TEST );
#endif

#ifdef ENABLE_FOG
	// enable depth fog
	GLfloat fogColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glEnable( GL_FOG );
	glFogi( GL_FOG_MODE, GL_LINEAR );
	glFogfv( GL_FOG_COLOR, fogColor );
	glHint( GL_FOG_HINT, GL_DONT_CARE );
	glFogf( GL_FOG_START, 2.0f );
	glFogf( GL_FOG_END, 5.0f );
#endif

	// set projection
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	//glOrtho( -SCREEN_WIDTH/2, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, -SCREEN_HEIGHT/2, -1, 1 );
	glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
	glFrustum( -0.5, 0.5, 0.375, -0.375, 1, 5 );

	// set base modelview matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glScalef( -1.0f / SCREEN_WIDTH, -1.0f / SCREEN_WIDTH, -1.0f );
	glTranslatef(0.0f, 0.0f, 1.0f);

	// return true if no errors
	return glGetError() == GL_NO_ERROR;
}

bool init()
{
	// initialize SDL
	if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
		return false;    

	// Check for joystick
	if (SDL_NumJoysticks() > 0)
	{
		// Open joystick
		SDL_Joystick *joy = SDL_JoystickOpen(0);
		if(joy)
		{
			DebugPrint("Opened Joystick 0\n");
			DebugPrint("Name: %s\n", SDL_JoystickName(0));
		}
	}

	// set OpenGL attributes
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
#ifdef ENABLE_SRC_ALPHA_SATURATE
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
#endif
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, MULTISAMPLE_BUFFERS );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, MULTISAMPLE_SAMPLES );

	// create the window
	if( SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, 0, SDL_OPENGL ) == NULL )
		return false;

	// initialize OpenGL
	if( !init_GL() )
		return false;    

	// set window title
	SDL_WM_SetCaption( "OpenGL Test", NULL );

#define TRACE_OPENGL_ATTRIBUTES
#ifdef TRACE_OPENGL_ATTRIBUTES
	int value;

	DebugPrint("Screen BPP: %d\n", SDL_GetVideoSurface()->format->BitsPerPixel);
	DebugPrint("\n");
	DebugPrint( "Vendor     : %s\n", glGetString( GL_VENDOR ) );
	DebugPrint( "Renderer   : %s\n", glGetString( GL_RENDERER ) );
	DebugPrint( "Version    : %s\n", glGetString( GL_VERSION ) );

	DebugPrint( "Extensions : \n" );
	const GLubyte *extensions = glGetString( GL_EXTENSIONS );
	char buf[4096];
	strncpy(buf, (const char *)extensions, sizeof(buf));
	char *extension = strtok(buf, " ");
	do
	{
		DebugPrint( "%s\n", extension );
		extension = strtok(NULL, " ");
	}
	while(extension);
	DebugPrint("\n");

	const char *attrib[] =
	{
		"SDL_GL_RED_SIZE",
		"SDL_GL_GREEN_SIZE",
		"SDL_GL_BLUE_SIZE",
		"SDL_GL_ALPHA_SIZE",
		"SDL_GL_BUFFER_SIZE",
		"SDL_GL_DOUBLEBUFFER",
		"SDL_GL_DEPTH_SIZE",
		"SDL_GL_STENCIL_SIZE",
		"SDL_GL_ACCUM_RED_SIZE",
		"SDL_GL_ACCUM_GREEN_SIZE",
		"SDL_GL_ACCUM_BLUE_SIZE",
		"SDL_GL_ACCUM_ALPHA_SIZE",
		"SDL_GL_STEREO",
		"SDL_GL_MULTISAMPLEBUFFERS",
		"SDL_GL_MULTISAMPLESAMPLES",
		"SDL_GL_ACCELERATED_VISUAL",
		"SDL_GL_SWAP_CONTROL"
	};
	for (int i = 0; i < SDL_arraysize(attrib); ++i)
	{
		SDL_GL_GetAttribute( SDL_GLattr(i), &value );
		DebugPrint( "%s: %d\n", attrib[i], value);
	}
#endif

	// success!
	return true;    
}

void clean_up()
{
	// quit SDL
	SDL_Quit();
}

#if 0
static const unsigned int sHashToAttribMask[][2] =
{
	{ 0xd965bbda /* "current" */,			GL_CURRENT_BIT },
	{ 0x18ae6c91 /* "point" */,				GL_POINT_BIT },
	{ 0x17db1627 /* "line" */,				GL_LINE_BIT },
	{ 0x051cb889 /* "polygon" */,			GL_POLYGON_BIT },
	{ 0x67b14997 /* "polygon_stipple" */,	GL_POLYGON_STIPPLE_BIT },
	{ 0xccde91eb /* "pixel_mode" */,		GL_LIGHTING_BIT },
	{ 0x827eb1c9 /* "lighting" */,			GL_POINT_BIT },
	{ 0xa1f3723f /* "fog" */,				GL_FOG_BIT },
	{ 0x65e5b825 /* "depth_buffer" */,		GL_DEPTH_BUFFER_BIT },
	{ 0x907f6213 /* "accum_buffer" */,		GL_ACCUM_BUFFER_BIT },
	{ 0x632020be /* "stencil_buffer" */,	GL_STENCIL_BUFFER_BIT },
	{ 0xe4abbac3 /* "viewport" */,			GL_VIEWPORT_BIT },
	{ 0xe1ad931b /* "transform" */,			GL_TRANSFORM_BIT },
	{ 0xaf8bb8ce /* "enable" */,			GL_ENABLE_BIT },
	{ 0x0d759bbb /* "color_buffer" */,		GL_COLOR_BUFFER_BIT },
	{ 0x4bc809b8 /* "hint" */,				GL_HINT_BIT },
	{ 0x08d22e0f /* "eval" */,				GL_EVAL_BIT },
	{ 0x0cfb5881 /* "list" */,				GL_LIST_BIT },
	{ 0x3c6468f4 /* "texture" */,			GL_TEXTURE_BIT },
	{ 0x0adbc081 /* "scissor" */,			GL_SCISSOR_BIT },
};
#endif

void ProcessDrawItems(TiXmlElement *element)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x974c9474 /* "pushmatrix" */:
			{
				glPushMatrix();
				ProcessDrawItems(child);
				glPopMatrix();
			}
			break;

		case 0x937cff81 /* "pushattrib" */:
			{
				GLuint mask = 0U;
				for (TiXmlAttribute *attrib = child->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
				{
					switch (Hash(attrib->Name()))
					{
					case 0xd965bbda /* "current" */:
						if (attrib->IntValue())
							mask |= GL_CURRENT_BIT;
						else
							mask &= ~GL_CURRENT_BIT;
						break;
					case 0x18ae6c91 /* "point" */:
						if (attrib->IntValue())
							mask |= GL_POINT_BIT;
						else
							mask &= ~GL_POINT_BIT;
						break;
					case 0x17db1627 /* "line" */:
						if (attrib->IntValue())
							mask |= GL_LINE_BIT;
						else
							mask &= ~GL_LINE_BIT;
						break;
					case 0x051cb889 /* "polygon" */:
						if (attrib->IntValue())
							mask |= GL_POLYGON_BIT;
						else
							mask &= ~GL_POLYGON_BIT;
						break;
					case 0x67b14997 /* "polygon_stipple" */:
						if (attrib->IntValue())
							mask |= GL_POLYGON_STIPPLE_BIT;
						else
							mask &= ~GL_POLYGON_STIPPLE_BIT;
						break;
					case 0xccde91eb /* "pixel_mode" */:
						if (attrib->IntValue())
							mask |= GL_LIGHTING_BIT;
						else
							mask &= ~GL_LIGHTING_BIT;
						break;
					case 0x827eb1c9 /* "lighting" */:
						if (attrib->IntValue())
							mask |= GL_POINT_BIT;
						else
							mask &= ~GL_POINT_BIT;
						break;
					case 0xa1f3723f /* "fog" */:
						if (attrib->IntValue())
							mask |= GL_FOG_BIT;
						else
							mask &= ~GL_FOG_BIT;
						break;
					case 0x65e5b825 /* "depth_buffer" */:
						if (attrib->IntValue())
							mask |= GL_DEPTH_BUFFER_BIT;
						else
							mask &= ~GL_DEPTH_BUFFER_BIT;
						break;
					case 0x907f6213 /* "accum_buffer" */:
						if (attrib->IntValue())
							mask |= GL_ACCUM_BUFFER_BIT;
						else
							mask &= ~GL_ACCUM_BUFFER_BIT;
						break;
					case 0x632020be /* "stencil_buffer" */:
						if (attrib->IntValue())
							mask |= GL_STENCIL_BUFFER_BIT;
						else
							mask &= ~GL_STENCIL_BUFFER_BIT;
						break;
					case 0xe4abbac3 /* "viewport" */:
						if (attrib->IntValue())
							mask |= GL_VIEWPORT_BIT;
						else
							mask &= ~GL_VIEWPORT_BIT;
						break;
					case 0xe1ad931b /* "transform" */:
						if (attrib->IntValue())
							mask |= GL_TRANSFORM_BIT;
						else
							mask &= ~GL_TRANSFORM_BIT;
						break;
					case 0xaf8bb8ce /* "enable" */:
						if (attrib->IntValue())
							mask |= GL_ENABLE_BIT;
						else
							mask &= ~GL_ENABLE_BIT;
						break;
					case 0x0d759bbb /* "color_buffer" */:
						if (attrib->IntValue())
							mask |= GL_COLOR_BUFFER_BIT;
						else
							mask &= ~GL_COLOR_BUFFER_BIT;
						break;
					case 0x4bc809b8 /* "hint" */:
						if (attrib->IntValue())
							mask |= GL_HINT_BIT;
						else
							mask &= ~GL_HINT_BIT;
						break;
					case 0x08d22e0f /* "eval" */:
						if (attrib->IntValue())
							mask |= GL_EVAL_BIT;
						else
							mask &= ~GL_EVAL_BIT;
						break;
					case 0x0cfb5881 /* "list" */:
						if (attrib->IntValue())
							mask |= GL_LIST_BIT;
						else
							mask &= ~GL_LIST_BIT;
						break;
					case 0x3c6468f4 /* "texture" */:
						if (attrib->IntValue())
							mask |= GL_TEXTURE_BIT;
						else
							mask &= ~GL_TEXTURE_BIT;
						break;
					case 0x0adbc081 /* "scissor" */:
						if (attrib->IntValue())
							mask |= GL_SCISSOR_BIT;
						else
							mask &= ~GL_SCISSOR_BIT;
						break;
					}
				}
				glPushAttrib(mask);
				ProcessDrawItems(child);
				glPopAttrib();
			}
			break;

		case 0x052eb8b2 /* "pushclientattrib" */:
			{
				GLuint mask = 0U;
				for (TiXmlAttribute *attrib = child->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
				{
					switch (Hash(attrib->Name()))
					{
					case 0x959fee19 /* "pixel_store" */:
						if (attrib->IntValue())
							mask |= GL_CLIENT_PIXEL_STORE_BIT;
						else
							mask &= ~GL_CLIENT_PIXEL_STORE_BIT;
						break;
					case 0x20a16825 /* "vertex_array" */:
						if (attrib->IntValue())
							mask |= GL_CLIENT_VERTEX_ARRAY_BIT;
						else
							mask &= ~GL_CLIENT_VERTEX_ARRAY_BIT;
						break;
					}
				}
				glPushClientAttrib(mask);
				ProcessDrawItems(child);
				glPopClientAttrib();
			}
			break;

		case 0xad0ecfd5 /* "translate" */:
			{
				float x = 0.0f, y = 0.0f, z = 0.0f;
				child->QueryFloatAttribute("x", &x);
				child->QueryFloatAttribute("y", &y);
				child->QueryFloatAttribute("z", &z);
				glTranslatef(x, y, z);
			}
			break;

		case 0xa5f4fd0a /* "rotate" */:
			{
				float a = 0.0f, x = 0.0f, y = 0.0f, z = 0.0f;
				child->QueryFloatAttribute("angle", &a);
				child->QueryFloatAttribute("x", &x);
				child->QueryFloatAttribute("y", &y);
				child->QueryFloatAttribute("z", &z);
				glRotatef(a, x, y, z);
			}
			break;

		case 0x82971c71 /* "scale" */:
			{
				float x = 1.0f, y = 1.0f, z = 1.0f;
				child->QueryFloatAttribute("x", &x);
				child->QueryFloatAttribute("y", &y);
				child->QueryFloatAttribute("z", &z);
				glScalef(x, y, z);
			}
			break;

		case 0x938fc4f7 /* "loadidentity" */:
			{
				glLoadIdentity();
			}
			break;

		case 0x7d22a710 /* "loadmatrix" */:
			{
				GLfloat m[16] = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				};
				for (int i = 0; i < 16; i++)
				{
					char name[16];
					sprintf(name, "m%d", i);
					child->QueryFloatAttribute(name, &m[i]);
				}
				glLoadMatrixf(m);
			}
			break;

		case 0x3807cb92 /* "multmatrix" */:
			{
				GLfloat m[16] = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				};
				for (int i = 0; i < 16; i++)
				{
					char name[16];
					sprintf(name, "m%d", i);
					child->QueryFloatAttribute(name, &m[i]);
				}
				glMultMatrixf(m);
			}
			break;

		case 0x945367a7 /* "vertex" */:
			{
				float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
				child->QueryFloatAttribute("x", &x);
				child->QueryFloatAttribute("y", &y);
				child->QueryFloatAttribute("z", &z);
				child->QueryFloatAttribute("w", &w);
				glVertex4f(x, y, z, w);
			}
			break;
		case 0xe68b9c52 /* "normal" */:
			{
				float x = 0.0f, y = 0.0f, z = 0.0f;
				child->QueryFloatAttribute("x", &x);
				child->QueryFloatAttribute("y", &y);
				child->QueryFloatAttribute("z", &z);
				glNormal3f(x, y, z);
			}
			break;

		case 0x3d7e6258 /* "color" */:
			{
				float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
				child->QueryFloatAttribute("r", &r);
				child->QueryFloatAttribute("g", &g);
				child->QueryFloatAttribute("b", &b);
				child->QueryFloatAttribute("a", &a);
				glColor4f(r, g, b, a);
			}
			break;

		case 0x090aa9ab /* "index" */:
			{
				float c = 0.0f;
				child->QueryFloatAttribute("c", &c);
				glIndexf(c);
			}
			break;

		case 0xdd612dd3 /* "texcoord" */:
			{
				float s = 0.0f, t = 0.0f, r = 0.0f, q = 1.0f;
				child->QueryFloatAttribute("s", &s);
				child->QueryFloatAttribute("t", &t);
				child->QueryFloatAttribute("r", &r);
				child->QueryFloatAttribute("q", &q);
				glTexCoord4f(s, t, r, q);
			}
			break;

		case 0x0135ab46 /* "edgeflag" */:
			{
				int flag;
				if (child->QueryIntAttribute("flag", &flag) == TIXML_SUCCESS)
					glEdgeFlag(flag ? GL_TRUE : GL_FALSE);
			}
			break;

		case 0x3c6468f4 /* "texture" */:
			break;

		case 0xbc9567c6 /* "points" */:
			{
				glBegin(GL_POINTS);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0xe1e4263c /* "lines" */:
			{
				glBegin(GL_LINES);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0xc2106ab6 /* "line_loop" */:
			{
				glBegin(GL_LINE_LOOP);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0xc6f2fa0e /* "line_strip" */:
			{
				glBegin(GL_LINE_STRIP);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0xd8a57342 /* "triangles" */:
			{
				glBegin(GL_TRIANGLES);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0x668b2dd8 /* "triangle_strip" */:
			{
				glBegin(GL_TRIANGLE_STRIP);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0xcfa6904f /* "triangle_fan" */:
			{
				glBegin(GL_TRIANGLE_FAN);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0x5667b307 /* "quads" */:
			{
				glBegin(GL_QUADS);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0xb47cad9b /* "quad_strip" */:
			{
				glBegin(GL_QUAD_STRIP);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0x051cb889 /* "polygon" */:
			{
				glBegin(GL_POLYGON);
				ProcessDrawItems(child);
				glEnd();
			}
			break;

		case 0xd2cf6b75 /* "calllist" */:
			{
				const char *name = child->Attribute("name");
				if (name)
				{
					DrawListMap::iterator itor = drawlists.find(Hash(name));
					if (itor != drawlists.end())
					{
						glCallList(itor->second);
					}
				}
			}
			break;

		case 0x2610a4a3 /* "clientstate" */:
			{
				for (TiXmlAttribute *attrib = child->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
				{
					switch (Hash(attrib->Name()))
					{
					case 0x945367a7 /* "vertex" */:
						if (attrib->IntValue())
							glEnableClientState(GL_VERTEX_ARRAY);
						else
							glDisableClientState(GL_VERTEX_ARRAY);
						break;
					case 0xe68b9c52 /* "normal" */:
						if (attrib->IntValue())
							glEnableClientState(GL_NORMAL_ARRAY);
						else
							glDisableClientState(GL_NORMAL_ARRAY);
						break;
					case 0x3d7e6258 /* "color" */:
						if (attrib->IntValue())
							glEnableClientState(GL_COLOR_ARRAY);
						else
							glDisableClientState(GL_COLOR_ARRAY);
						break;
					case 0x090aa9ab /* "index" */:
						if (attrib->IntValue())
							glEnableClientState(GL_INDEX_ARRAY);
						else
							glDisableClientState(GL_INDEX_ARRAY);
						break;
					case 0xdd612dd3 /* "texcoord" */:
						if (attrib->IntValue())
							glEnableClientState(GL_TEXTURE_COORD_ARRAY);
						else
							glDisableClientState(GL_TEXTURE_COORD_ARRAY);
						break;
					case 0x0135ab46 /* "edgeflag" */:
						if (attrib->IntValue())
							glEnableClientState(GL_EDGE_FLAG_ARRAY);
						else
							glDisableClientState(GL_EDGE_FLAG_ARRAY);
						break;
					}
				}
			}
			break;

		case 0x6298bce4 /* "vertexarray" */:
			{
				int size = 0;
				child->QueryIntAttribute("size", &size);

				int stride = 0;
				child->QueryIntAttribute("stride", &stride);

				const char *text = child->GetText();
				size_t len = strlen(text)+1;
				char *buf = static_cast<char *>(_alloca(len));
				memcpy(buf, text, len);
				float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
				int count = 0;
				char *element = strtok(buf, " \t\n\r,;");
				while (element)
				{
					data[count++] = float(atof(element));
					element = strtok(NULL, " \t\n\r,;");
				}

				glVertexPointer(size, GL_FLOAT, stride, data);
			}
			break;

		case 0x81491d33 /* "normalarray" */:
			{
				int stride = 0;
				child->QueryIntAttribute("stride", &stride);

				const char *text = child->GetText();
				size_t len = strlen(text)+1;
				char *buf = static_cast<char *>(_alloca(len));
				memcpy(buf, text, len);
				float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
				int count = 0;
				char *element = strtok(buf, " \t\n\r,;");
				while (element)
				{
					data[count++] = float(atof(element));
					element = strtok(NULL, " \t\n\r,;");
				}

				glNormalPointer(GL_FLOAT, stride, data);
			}
			break;

		case 0xcce5b995 /* "colorarray" */:
			{
				int size = 0;
				child->QueryIntAttribute("size", &size);

				int stride = 0;
				child->QueryIntAttribute("stride", &stride);

				const char *text = child->GetText();
				size_t len = strlen(text)+1;
				char *buf = static_cast<char *>(_alloca(len));
				memcpy(buf, text, len);
				float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
				int count = 0;
				char *element = strtok(buf, " \t\n\r,;");
				while (element)
				{
					data[count++] = float(atof(element));
					element = strtok(NULL, " \t\n\r,;");
				}

				glColorPointer(size, GL_FLOAT, stride, data);
			}
			break;

		case 0x664ead80 /* "indexarray" */:
			{
				int stride = 0;
				child->QueryIntAttribute("stride", &stride);

				const char *text = child->GetText();
				size_t len = strlen(text)+1;
				char *buf = static_cast<char *>(_alloca(len));
				memcpy(buf, text, len);
				float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
				int count = 0;
				char *element = strtok(buf, " \t\n\r,;");
				while (element)
				{
					data[count++] = float(atof(element));
					element = strtok(NULL, " \t\n\r,;");
				}

				glIndexPointer(GL_FLOAT, stride, data);
			}
			break;

		case 0x91aa3148 /* "texcoordarray" */:
			{
				int size = 0;
				child->QueryIntAttribute("size", &size);

				int stride = 0;
				child->QueryIntAttribute("stride", &stride);

				const char *text = child->GetText();
				size_t len = strlen(text)+1;
				char *buf = static_cast<char *>(_alloca(len));
				memcpy(buf, text, len);
				float *data = static_cast<float *>(_alloca(len*sizeof(float)/2));
				int count = 0;
				char *element = strtok(buf, " \t\n\r,;");
				while (element)
				{
					data[count++] = float(atof(element));
					element = strtok(NULL, " \t\n\r,;");
				}

				glTexCoordPointer(size, GL_FLOAT, stride, data);
			}
			break;

		case 0x60360ccf /* "edgeflagarray" */:
			{
				int stride = 0;
				child->QueryIntAttribute("stride", &stride);

				const char *text = child->GetText();
				size_t len = strlen(text)+1;
				char *buf = static_cast<char *>(_alloca(len));
				memcpy(buf, text, len);
				bool *data = static_cast<bool *>(_alloca(len*sizeof(bool)/2));
				int count = 0;
				char *element = strtok(buf, " \t\n\r,;");
				while (element)
				{
					data[count++] = atoi(element) != 0;
					element = strtok(NULL, " \t\n\r,;");
				}

				glEdgeFlagPointer(stride, data);
			}
			break;

		case 0x0a85bb5e /* "arrayelement" */:
			{
				int index;
				if (child->QueryIntAttribute("index", &index) == TIXML_SUCCESS)
					glArrayElement(index);
			}
			break;

		case 0xf4de4a21 /* "drawarrays" */:
			{
				GLenum mode;
				switch (Hash(child->Attribute("mode")))
				{
				case 0xbc9567c6 /* "points" */:			mode = GL_POINTS; break;
				case 0xe1e4263c /* "lines" */:			mode = GL_LINES; break;
				case 0xc2106ab6 /* "line_loop" */:		mode = GL_LINE_LOOP; break;
				case 0xc6f2fa0e /* "line_strip" */:		mode = GL_LINE_STRIP; break;
				case 0xd8a57342 /* "triangles" */:		mode = GL_TRIANGLES; break;
				case 0x668b2dd8 /* "triangle_strip" */:	mode = GL_TRIANGLE_STRIP; break;
				case 0xcfa6904f /* "triangle_fan" */:	mode = GL_TRIANGLE_FAN; break;
				case 0x5667b307 /* "quads" */:			mode = GL_QUADS; break;
				case 0xb47cad9b /* "quad_strip" */:		mode = GL_QUAD_STRIP; break;
				case 0x051cb889 /* "polygon" */:		mode = GL_POLYGON; break;
				default: break;
				}

				int first = 0, count = 0;
				child->QueryIntAttribute("first", &first);
				child->QueryIntAttribute("count", &count);
				glDrawArrays(mode, first, count);
			}
			break;

		case 0x757eeee2 /* "drawelements" */:
			{
				GLenum mode;
				switch (Hash(child->Attribute("mode")))
				{
				case 0xbc9567c6 /* "points" */:			mode = GL_POINTS; break;
				case 0xe1e4263c /* "lines" */:			mode = GL_LINES; break;
				case 0xc2106ab6 /* "line_loop" */:		mode = GL_LINE_LOOP; break;
				case 0xc6f2fa0e /* "line_strip" */:		mode = GL_LINE_STRIP; break;
				case 0xd8a57342 /* "triangles" */:		mode = GL_TRIANGLES; break;
				case 0x668b2dd8 /* "triangle_strip" */:	mode = GL_TRIANGLE_STRIP; break;
				case 0xcfa6904f /* "triangle_fan" */:	mode = GL_TRIANGLE_FAN; break;
				case 0x5667b307 /* "quads" */:			mode = GL_QUADS; break;
				case 0xb47cad9b /* "quad_strip" */:		mode = GL_QUAD_STRIP; break;
				case 0x051cb889 /* "polygon" */:		mode = GL_POLYGON; break;
				default: break;
				}

				const char *text = child->GetText();
				size_t len = strlen(text)+1;
				char *buf = static_cast<char *>(_alloca(len));
				memcpy(buf, text, len);
				unsigned short *indices = static_cast<unsigned short *>(_alloca(len*sizeof(unsigned short)/2));
				int count = 0;
				char *element = strtok(buf, " \t\n\r,;");
				while (element)
				{
					indices[count++] = unsigned short(atoi(element));
					element = strtok(NULL, " \t\n\r,;");
				}

				glDrawElements(mode, count, GL_UNSIGNED_SHORT, indices);
			}
			break;

		default:
			break;
		}
	}
}

static void ProcessEntityItems(TiXmlElement *element, Entity *entity)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		entity->Configure(child);
	}
}

static void ProcessWorldItems(TiXmlElement *element)
{
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *value = child->Value();
		switch (Hash(value))
		{
		case 0xd33ff5da /* "entity" */:
			{
				// entity
				Entity *entity = NULL;
				const char *name = child->Attribute("name");
				const char *type = child->Attribute("type");
				switch (Hash(type))
				{
				case 0x1ac6a97e /* "cloud" */:
					{
						int count = 1;
						child->QueryIntAttribute("count", &count);
						entity = new Cloud(count);
					}
					break;

				case 0xaf871a91 /* "grid" */:
					entity = new Grid();
					break;

				case 0x2c99c300 /* "player" */:
					entity = new Player();
					break;

				case 0xe063cbaa /* "gunner" */:
					entity = new Gunner();
					break;

				case 0x32608848 /* "target" */:
					entity = new Target();
					break;

				default:
					break;
				}

				if (entity)
				{
					// register with the entity map
					entities[Hash(name)] = entity;

					// process entity items
					ProcessEntityItems(child, entity);
				}
			}
			break;

		case 0xc98b019b /* "drawlist" */:
			{
				// create a new draw list
				GLuint handle = glGenLists(1);
				glNewList(handle, GL_COMPILE);

				// get the list name
				const char *name = child->Attribute("name");
				if (name)
				{
					// register the draw list
					drawlists[Hash(name)] = handle;
				}

				// process draw items
				ProcessDrawItems(child);

				// finish the draw list
				glEndList();
			}
			break;

		default:
			break;
		}
	}
}

enum InputType
{
	INPUT_TYPE_KEYBOARD,
	INPUT_TYPE_MOUSE_AXIS,
	INPUT_TYPE_MOUSE_BUTTON,
	INPUT_TYPE_JOYSTICK_AXIS,
	INPUT_TYPE_JOYSTICK_BUTTON,
	NUM_INPUT_TYPES
};

// main
int SDL_main( int argc, char *argv[] )
{
	// quit flag
	bool quit = false;

	// initialize
	if( !init() )
		return 1;    

	// set collision layers
	Collidable::SetLayerMask(COLLISION_LAYER_PLAYER_BULLET, 1<<2);

	Input input;

	{
		// input binding
		TiXmlDocument config("input.xml");
		config.LoadFile();

		TiXmlHandle handle( &config );
		TiXmlElement *element = handle.FirstChildElement("input").ToElement();
		if (element)
		{
			for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				const char *value = child->Value();
				switch (Hash(value))
				{
				case 0xc7535f2e /* "bind" */:
					{
						// map logical name
						const char *name = child->Attribute("name");
						Input::LOGICAL logical;
						switch(Hash(name))
						{
						case 0x2f7d674b /* "move_x" */:	logical = Input::MOVE_HORIZONTAL; break;
						case 0x2e7d65b8 /* "move_y" */: logical = Input::MOVE_VERTICAL; break;
						case 0x28e0ac09 /* "aim_x" */:	logical = Input::AIM_HORIZONTAL; break;
						case 0x27e0aa76 /* "aim_y" */:	logical = Input::AIM_VERTICAL; break;
						case 0x8eab16d9 /* "fire" */:	logical = Input::FIRE_PRIMARY; break;
						default:						logical = Input::NUM_LOGICAL; break;
						}

						// map input type
						const char *type = child->Attribute("type");
						InputType inputtype;
						switch(Hash(type))
						{
						case 0x4aa845f4 /* "keyboard" */:			inputtype = INPUT_TYPE_KEYBOARD; break;
						case 0xd76afdc0 /* "mouse_axis" */:			inputtype = INPUT_TYPE_MOUSE_AXIS; break;
						case 0xbe730575 /* "mouse_button" */:		inputtype = INPUT_TYPE_MOUSE_BUTTON; break;
						case 0x4b1fb051 /* "joystick_axis" */:		inputtype = INPUT_TYPE_JOYSTICK_AXIS; break;
						case 0xb084d264 /* "joystick_button" */:	inputtype = INPUT_TYPE_JOYSTICK_BUTTON; break;
						default:									inputtype = NUM_INPUT_TYPES; break;
						}

						// get properties
						int device = 0;
						child->QueryIntAttribute("device", &device);
						int control = 0;
						child->QueryIntAttribute("control", &control);
						float deadzone = 0.0f;
						child->QueryFloatAttribute("deadzone", &deadzone);
						float scale = 1.0f;
						child->QueryFloatAttribute("scale", &scale);

						input.Bind(logical, inputtype, device, control, deadzone, scale);
					}
					break;
				}
			}
		}
	}

	{
		// level configuration
		TiXmlDocument level("level.xml");
		level.LoadFile();

		// process child elements of world
		TiXmlHandle handle( &level );
		TiXmlElement *world = handle.FirstChildElement("world").ToElement();
		if (world)
		{
			ProcessWorldItems(world);
		}
	}

	// find the player
	Player *player = NULL;
	{
		EntityMap::iterator itor = entities.find(Hash("player"));
		if (itor != entities.end())
		{
			player = dynamic_cast<Player *>(itor->second);
			player->SetInput(&input);
		}
	}

    // timer timer
    Timer timer;
    
    // start timer
    timer.start();

	// last ticks
	int ticks = timer.get_ticks();

	// wait for user exit
	do
	{
		// INPUT PHASE

		// start input
		input.Start();

		// event handler
		SDL_Event event;

		// process events
		while( SDL_PollEvent( &event ) )
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				input.OnPress( INPUT_TYPE_KEYBOARD, event.key.which, event.key.keysym.sym );
				if ((event.key.keysym.sym == SDLK_F4) && (event.key.keysym.mod & KMOD_ALT))
				{
					quit = true;
				}
				else if (event.key.keysym.sym == SDLK_PAUSE)
				{
					if (timer.is_paused())
						timer.unpause();
					else
						timer.pause();
				}
				break;
			case SDL_KEYUP:
				input.OnRelease( INPUT_TYPE_KEYBOARD, event.key.which, event.key.keysym.sym );
				break;
			case SDL_MOUSEMOTION:
				input.OnAxis( INPUT_TYPE_MOUSE_AXIS, event.motion.which, 0, float(event.motion.x - SCREEN_WIDTH/2) );
				input.OnAxis( INPUT_TYPE_MOUSE_AXIS, event.motion.which, 1, float(event.motion.y - SCREEN_HEIGHT/2) );
				break;
			case SDL_MOUSEBUTTONDOWN:
				input.OnPress( INPUT_TYPE_MOUSE_BUTTON, event.button.which, event.button.button );
				break;
			case SDL_MOUSEBUTTONUP:
				input.OnRelease( INPUT_TYPE_MOUSE_BUTTON, event.button.which, event.button.button );
				break;
			case SDL_JOYAXISMOTION:
				input.OnAxis( INPUT_TYPE_JOYSTICK_AXIS, event.jaxis.which, event.jaxis.axis, event.jaxis.value / 32767.0f );
				break;
			case SDL_JOYBUTTONDOWN:
				input.OnPress( INPUT_TYPE_JOYSTICK_BUTTON, event.jaxis.which, event.jbutton.button );
				break;
			case SDL_JOYBUTTONUP:
				input.OnRelease( INPUT_TYPE_JOYSTICK_BUTTON, event.jbutton.which, event.jbutton.button );
				break;
			case SDL_QUIT:
				quit = true;
				break;
			}
		}


		// get loop time in ticks
		int delta = timer.get_ticks() - ticks;
		ticks += delta;

		// if not paused...
		if (delta != 0)
		{
			// get loop time in seconds
			float step = delta / 1000.0f;
#ifdef PRINT_STEP_TIMES
			DebugPrint("dt=%f (%f fps)\n", step, 1.0f/step);
#endif

			// CONTROL PHASE

			// control all entities
			Controllable::ControlAll(step);


			// COLLISION PHASE

			// collide all entities
			Collidable::CollideAll(step);


			// SIMULATION PHASE

			// simulate all entities
			Simulatable::SimulateAll(step);
		}


		// RENDERING PHASE

		// clear the screen
		glClear(
			GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_BUFFER
			| GL_DEPTH_BUFFER_BIT
#endif
			);

		// push camera transform
		glPushMatrix();

		if (player)
		{
			// track player position
			const Vector2 &pos = player->GetPosition();
			glTranslatef( -pos.x, -pos.y, 0 );
		}

		// render all entities
		Renderable::RenderAll();

		// reset camera transform
		glPopMatrix();

		// show the screen
		SDL_GL_SwapBuffers();
	}
	while( !quit );

	// clean up
	clean_up();

	// done
	return 0;
}
