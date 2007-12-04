// precompiled header
#include "stdafx.h"

// includes
#include "Timer.h"
#include "Cloud.h"
#include "Grid.h"
#include "Player.h"
#include "Gunner.h"
#include "Target.h"
#include "Bullet.h"
#include "Explosion.h"

// screen attributes
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
bool SCREEN_FULLSCREEN = false;

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
	glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
	glFrustum( -0.5, 0.5, 0.5f*SCREEN_HEIGHT/SCREEN_WIDTH, -0.5f*SCREEN_HEIGHT/SCREEN_WIDTH, 1, 5 );

	// set base modelview matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glScalef( -1.0f / 640, -1.0f / 640, -1.0f );
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
	unsigned int flags = SDL_OPENGL;
	if (SCREEN_FULLSCREEN)
		flags |= SDL_FULLSCREEN;
	if( SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, 0, flags ) == NULL )
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
class Loader : public TiXmlVisitor
{
	// visit a document
	virtual bool VisitEnter( const TiXmlDocument& doc );
	virtual bool VisitExit( const TiXmlDocument& doc )

	// visit an element
	virtual bool VisitEnter( const TiXmlElement& element, const TiXmlAttribute* firstAttribute );
	virtual bool VisitExit( const TiXmlElement& element );

	// visit a text node
	virtual bool Visit( const TiXmlText& text );
};

bool Loader::VisitEnter( const TiXmlDocument& doc )
{
	return true;
}

bool Loader::VisitExit( const TiXmlDocument& doc )
{
	return true;
}

bool Loader::VisitEnter( const TiXmlElement& element, const TiXmlAttribute* firstAttribute )
{
	return true;
}

bool Loader::VisitExit( const TiXmlElement& element )
{
	return true;
}
#endif

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
					GLuint drawlist = Database::drawlist.Get(Hash(name));
					if (drawlist)
					{
						glCallList(drawlist);
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

static void ProcessEntityItems(TiXmlElement *element)
{
	// entity
	Entity *entity = NULL;

	// get entity identifier
	const char *name = element->Attribute("name");
	unsigned int entity_id = Hash(name);

	// get parent identifier
	const char *type = element->Attribute("type");
	unsigned int parent_id = Hash(type);

	// create entity based on parent identifier (HACK)
	switch (parent_id)
	{
	case 0x1ac6a97e /* "cloud" */:
		{
			Cloud *cloud = new Cloud(entity_id, parent_id);

			int count = 1;
			element->QueryIntAttribute("count", &count);
			cloud->Init(count);

			entity = cloud;
		}
		break;

	case 0xaf871a91 /* "grid" */:
		entity = new Grid(entity_id, parent_id);
		break;

	case 0x2c99c300 /* "player" */:
		entity = new Player(entity_id, parent_id);
		break;

	case 0xe063cbaa /* "gunner" */:
		entity = new Gunner(entity_id, parent_id);
		break;

	case 0x32608848 /* "target" */:
	case 0xa7262d15 /* "wall" */:
		entity = new Target(entity_id, parent_id);
		break;

	default:
		break;
	}

	if (entity)
	{
		// set parent
		Database::parent.Put(entity_id, parent_id);

		// add to entity map
		Database::entity.Put(entity_id, entity);

		// process child elements
		for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
		{
			entity->Configure(child);
		}
		
		// initialize the entity (HACK)
		Collidable *collidable = dynamic_cast<Collidable *>(entity);
		if (collidable)
			collidable->AddToWorld();

		entity->Init();
	}
}

static void ProcessTemplateItems(TiXmlElement *element)
{
	// get template identifier
	const char *name = element->Attribute("name");
	unsigned int template_id = Hash(name);

	// get parent identifier
	const char *type = element->Attribute("type");
	unsigned int parent_id = Hash(type);

	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *value = child->Value();
		switch (Hash(value))
		{
		case 0x74e9dbae /* "collidable" */:
			{
				CollidableTemplate collidable;
				collidable.Configure(child);
				Database::collidabletemplate.Put(template_id, collidable);
			}
			break;

		case 0x109dd1ad /* "renderable" */:
			{
				RenderableTemplate renderable;
				renderable.Configure(child);
				Database::renderabletemplate.Put(template_id, renderable);
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
					Database::drawlist.Put(Hash(name), handle);
				}

				// process draw items
				ProcessDrawItems(child);

				// finish the draw list
				glEndList();
			}
			break;
		}
	}

	Database::parent.Put(template_id, parent_id);
}

static void ProcessWorldItems(TiXmlElement *element)
{
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *value = child->Value();
		switch (Hash(value))
		{
		case 0x694aaa0b /* "template" */:
			{
				ProcessTemplateItems(child);
			}
			break;

		case 0xd33ff5da /* "entity" */:
			{
				// process entity items
				ProcessEntityItems(child);
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
					Database::drawlist.Put(Hash(name), handle);
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
	// default input configuration
	const char *inputconfig = "input.xml";

	// default level configuration
	const char *levelconfig = "level.xml";

	// process command-line arguments
	for (int i = 1; i < argc; ++i)
	{
		switch (Hash(argv[i]))
		{
		case 0x15a05e8a /* "-resolution" */:
			SCREEN_WIDTH = atoi(argv[++i]);
			SCREEN_HEIGHT = atoi(argv[++i]);
			break;

		case 0x0d138139 /* "-fullscreen" */:
			SCREEN_FULLSCREEN = true;
			break;

		case 0x183ca255 /* "-windowed" */:
			SCREEN_FULLSCREEN = false;
			break;

		case 0xb6627acc /* "-input" */:
			inputconfig = argv[++i];
			break;

		case 0x1b4dcd6e /* "-level" */:
			levelconfig = argv[++i];
			break;
		}
	}

	// quit flag
	bool quit = false;

	// initialize
	if( !init() )
		return 1;    

	// input system
	Input input;

	{
		// input binding
		TiXmlDocument document(inputconfig);
		document.LoadFile();

		TiXmlHandle handle( &document );
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

	// collidable initialization
	Collidable::WorldInit();

	{
		// level configuration
		TiXmlDocument document(levelconfig);
		document.LoadFile();

		// process child elements of world
		TiXmlHandle handle( &document );
		TiXmlElement *element = handle.FirstChildElement("world").ToElement();
		if (element)
		{
			ProcessWorldItems(element);
		}
	}

	// find the player
	Player *player = dynamic_cast<Player *>(Database::entity.Get(Hash("playership")));
	if (player)
	{
		player->SetInput(&input);
	}

    // timer timer
    Timer timer;
    
    // start timer
    timer.start();

	// last ticks
	int ticks = timer.get_ticks();

	// simulation timer
	static const float sim_rate = 60.0f;
	const float sim_step = 1.0f / sim_rate;
	float sim_timer = 1.0f;

	// wait for user exit
	do
	{
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

		// clamp ticks to something sensible
		// (while debugging, for example)
		if (delta > 1000)
			delta = 1000;

		// advance the sim timer
		sim_timer += delta * sim_rate / 1000.0f;

		// while simulation turns to run...
		while (sim_timer > 1.0f)
		{
			// deduct a turn
			sim_timer -= 1.0f;


			// INPUT PHASE

			// update inputs
			input.Update();


			// CONTROL PHASE

#ifdef PRINT_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_freq;
			QueryPerformanceFrequency(&perf_freq);

			LARGE_INTEGER perf_count0;
			QueryPerformanceCounter(&perf_count0);
#endif

			// control all entities
			Controllable::ControlAll(sim_step);

#ifdef PRINT_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_count1;
			QueryPerformanceCounter(&perf_count1);

			DebugPrint("ctrl=%d ", 1000000 * (perf_count1.QuadPart - perf_count0.QuadPart) / perf_freq.QuadPart);
#endif

			// SIMULATION PHASE
			// (generate forces)

			// simulate all entities
			Simulatable::SimulateAll(sim_step);

#ifdef PRINT_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_count2;
			QueryPerformanceCounter(&perf_count2);

			DebugPrint("simu=%d ", 1000000 * (perf_count2.QuadPart - perf_count1.QuadPart) / perf_freq.QuadPart);
#endif

			// COLLISION PHASE
			// (apply forces and update positions)
			Collidable::CollideAll(sim_step);

#ifdef PRINT_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_count3;
			QueryPerformanceCounter(&perf_count3);

			DebugPrint("coll=%d ", 1000000 * (perf_count3.QuadPart - perf_count2.QuadPart) / perf_freq.QuadPart);
#endif
		}

#ifdef PRINT_SIMULATION_TIMER
		DebugPrint("delta=%d ticks=%d sim_t=%f\n", delta, ticks, sim_timer);
#endif

		// RENDERING PHASE

#ifdef PRINT_PERFORMANCE_DETAILS
		LARGE_INTEGER perf_freq;
		QueryPerformanceFrequency(&perf_freq);

		LARGE_INTEGER perf_count0;
		QueryPerformanceCounter(&perf_count0);
#endif
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
			const Vector2 pos(player->GetInterpolatedPosition(sim_timer));
			glTranslatef( -pos.x, -pos.y, 0 );
		}

		// render all entities
		Renderable::RenderAll(sim_timer);

		// reset camera transform
		glPopMatrix();

		// show the screen
		SDL_GL_SwapBuffers();

#ifdef PRINT_PERFORMANCE_DETAILS
		LARGE_INTEGER perf_count1;
		QueryPerformanceCounter(&perf_count1);

		DebugPrint("rend=%d\n", 1000000 * (perf_count1.QuadPart - perf_count0.QuadPart) / perf_freq.QuadPart);
#endif
	}
	while( !quit );

	// clear pools
	Bullet::pool.~object_pool();
	Explosion::pool.~object_pool();

	// remove all entities
	for (Database::Typed<Entity *>::iterator itor = Database::entity.begin(); itor != Database::entity.end(); ++itor)
		delete itor->second;
	Database::entity.clear();

	// remove all drawlists
	for (Database::Typed<GLuint>::iterator itor = Database::drawlist.begin(); itor != Database::drawlist.end(); ++itor)
		glDeleteLists(itor->second, 1);
	Database::drawlist.clear();

	// clear databases
	Database::collidabletemplate.clear();
	Database::collidable.clear();
	Database::renderable.clear();

	// collidable done
	Collidable::WorldDone();

	// clean up
	clean_up();

	// done
	return 0;
}
