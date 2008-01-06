// precompiled header
#include "stdafx.h"

// includes
#include "Timer.h"
#include "Cloud.h"
#include "Player.h"
#include "Aimer.h"
#include "Ship.h"
#include "Gunner.h"
#include "Weapon.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Damagable.h"
#include "Spawner.h"
#include "Link.h"
#include "Interpolator.h"

// screen attributes
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
int SCREEN_DEPTH = 0;
bool SCREEN_FULLSCREEN = false;

// opengl attributes
bool OPENGL_DOUBLEBUFFER = true;
bool OPENGL_STEREO = false;
bool OPENGL_ACCELERATED = true;
bool OPENGL_SWAPCONTROL = true;
bool OPENGL_ANTIALIAS = false;
int OPENGL_MULTISAMPLE = 16;

// simulation attributes
int SIMULATION_RATE = 60;
float TIME_SCALE = 1.0f;

// input system
Input input;

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

	if (OPENGL_ANTIALIAS)
	{
		// enable point smoothing
		glEnable( GL_POINT_SMOOTH );
		glHint( GL_POINT_SMOOTH_HINT, GL_DONT_CARE );

		// enable line smoothing
 		glEnable( GL_LINE_SMOOTH );
		glHint( GL_LINE_SMOOTH_HINT, GL_DONT_CARE );

		// enable polygon smoothing
		glEnable( GL_POLYGON_SMOOTH );
		glHint( GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE );
	}

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
	glTranslatef( 0.0f, 0.0f, 1.0f );

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
#else
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
#endif
#ifndef ENABLE_DEPTH_TEST
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
#endif
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, OPENGL_DOUBLEBUFFER );
	SDL_GL_SetAttribute( SDL_GL_STEREO, OPENGL_STEREO );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, OPENGL_MULTISAMPLE > 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, OPENGL_MULTISAMPLE );
//	SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, OPENGL_ACCELERATED );		// this breaks multisampling for some reason...
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, OPENGL_SWAPCONTROL );

	// create the window
	unsigned int flags = SDL_OPENGL;
	if (SCREEN_FULLSCREEN)
		flags |= SDL_FULLSCREEN;
	if( SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, flags ) == NULL )
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

size_t ExecuteDrawDataDeferred(const unsigned int buffer[], size_t count, int width, float data[], float param)
{
	int index = 0;
	switch(buffer[index++])
	{
	case 0x1dff06ae /* "global" */:
		{
			// TO DO: get the global value
			unsigned int name = buffer[index++];
		}
		break;

	case 0x83588fd4 /* "interpolator" */:
		{
			unsigned int size = buffer[index++];

			// get interpolator value
			InterpolatorTemplate interpolator(width);
			interpolator.mCount = buffer[index];
			interpolator.mKeys = (float *)&buffer[index+1];
			int dummy = 0;
			if (!interpolator.Apply(data, param, dummy))
				memset(data, 0, width*sizeof(float));
			interpolator.mKeys = NULL;

			index += size;
		}
		break;

	case 0xecb9d8e4 /* "literal" */:
		{
			// get literal value
			for (int i = 0; i < width; i++)
				data[i] = *reinterpret_cast<const float *>(&buffer[index++]);
		}
		break;

	default:
		break;
	}

	return index;
}

void ExecuteDeferredDrawItems(const unsigned int buffer[], size_t count, float param)
{
	GLfloat data[4];

	const unsigned int *itor = buffer;
	while (itor < buffer + count)
	{
		switch (*itor++)
		{
		case 0xf6604733 /* "glPushMatrix" */:
			glPushMatrix();
			break;

		case 0xfc8a1d94 /* "glPopMatrix" */:
			glPopMatrix();
			break;

		case 0xa471ec02 /* "glPushAttrib" */:
			glPushAttrib(GLbitfield(*itor++));
			break;

		case 0x73c4cda1 /* "glPopAttrib" */:
			glPopAttrib();
			break;

		case 0x485249b9 /* "glPushClientAttrib" */:
			glPushClientAttrib(GLbitfield(*itor++));
			break;

		case 0xbfd4add2 /* "glPopClientAttrib" */:
			glPopClientAttrib();
			break;

		case 0xafeef11e /* "glTranslatef" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 3, data, param);
			glTranslatef(data[0], data[1], data[2]);
			break;

		case 0x29e02ba1 /* "glRotatef" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 4, data, param);
			glRotatef(data[0], data[1], data[2], data[3]);
			break;

		case 0xff71cf6e /* "glScalef" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 3, data, param);
			glScalef(data[0], data[1], data[2]);
			break;

		case 0xcbd6bd5c /* "glLoadIdentity" */:
			glLoadIdentity();
			break;

		case 0xca9090d7 /* "glLoadMatrixf" */:
			glLoadMatrixf(reinterpret_cast<const GLfloat *>(&*itor));
			itor+=16;
			break;

		case 0x64500671 /* "glMultMatrixf" */:
			glMultMatrixf(reinterpret_cast<const GLfloat *>(&*itor));
			itor+=16;
			break;

		case 0x94110c7a /* "glVertex4f" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 4, data, param);
			glVertex4fv(data);
			break;

		case 0xf2d58094 /* "glNormal3f" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 3, data, param);
			glNormal3fv(data);
			break;

		case 0x9d63d16b /* "glColor4f" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 4, data, param);
			glColor4fv(data);
			break;

		case 0xf3b3b82c /* "glIndexf" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 1, data, param);
			glIndexf(data[0]);
			break;

		case 0xb78bb2ae /* "glTexCoord4f" */:
			itor += ExecuteDrawDataDeferred(itor, buffer + count - itor, 4, data, param);
			glTexCoord4fv(data);
			break;

		case 0x7f5dcd49 /* "glEdgeFlag" */:
			glEdgeFlag(*itor++ != 0);
			break;

		case 0xb70e76e3 /* "glBegin" */:
			glBegin(GLenum(*itor++));
			break;

		case 0x50257afb /* "glEnd" */:
			glEnd();
			break;

		case 0x9525d6fe /* "glCallList" */:
			glCallList(GLuint(*itor++));
			break;

		case 0x9128677b /* "glEnableClientState" */:
			glEnableClientState(GLenum(*itor++));
			break;

		case 0x342d0316 /* "glDisableClientState" */:
			glDisableClientState(GLenum(*itor++));
			break;

		case 0x4e467465 /* "glVertexPointer" */:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glVertexPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x46804012 /* "glNormalPointer" */:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glNormalPointer(GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x61e8560e /* "glColorPointer" */:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glColorPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x1e5cf423 /* "glIndexPointer" */:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glIndexPointer(GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x6d976421 /* "glTexCoordPointer" */:
			{
				GLint size = *itor++;
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glTexCoordPointer(size, GL_FLOAT, stride, &*itor);
				itor += count;
			}
			break;

		case 0x9cfbc596 /* "glEdgeFlagPointer" */:
			{
				GLsizei stride = *itor++;
				size_t count = *itor++;
				glEdgeFlagPointer(stride, &*itor);
				itor += count*sizeof(bool)/sizeof(unsigned int);
			}
			break;

		case 0x8cfc8329 /* "glArrayElement" */:
			glArrayElement(*itor++);
			break;

		case 0x806f1b62 /* "glDrawArrays" */:
			{
				GLenum mode = *itor++;
				GLint first = *itor++;
				size_t count = *itor++;
				glDrawArrays(mode, first, count);
			}
			break;

		case 0xf6e885d9 /* "glDrawElements" */:
			{
				GLenum mode = *itor++;
				GLsizei count = *itor++;
				glDrawElements(mode, count, GL_UNSIGNED_SHORT, &*itor);
				itor += count*sizeof(unsigned short)/sizeof(unsigned int);
			}
			break;

		case 0xd99ba82a /* "repeat" */:
			{
				int repeat = *itor++;
				int length = *itor++;
				for (int i = 0; i < repeat; i++)
					ExecuteDeferredDrawItems(itor, length, param);
				itor += length;
			}
			break;

		default:
			break;
		}
	}
}


// attribute names
static const char * sPositionNames[] = { "x", "y", "z", "w" };
static const char * sRotationNames[] = { "angle", "x", "y", "z" };
static const char * sColorNames[] = { "r", "g", "b", "a" };
static const char * sTexCoordNames[] = { "s", "t", "r", "q" };
static const char * sIndexNames[] = { "c" };
static const char * sMatrixNames[] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15" };

void ProcessDrawDataDeferred(TiXmlElement *element, unsigned int id, std::vector<unsigned int> &buffer, int width, const char *names[], const float data[])
{
	if (const char *name = element->Attribute("name"))
	{
		// push a reference to a global value
		buffer.push_back(0x1dff06ae /* "global" */);
		buffer.push_back(Hash(name));
	}
	else if (element->FirstChildElement())
	{
		// push an interpolator
		buffer.push_back(0x83588fd4 /* "interpolator" */);
		buffer.push_back(0);
		int start = buffer.size();
		ProcessInterpolatorItem(element, buffer, width, names, data);
		buffer[start - 1] = buffer.size() - start;
	}
	else
	{
		// push literal data
		buffer.push_back(0xecb9d8e4 /* "literal" */);
		for (int i = 0; i < width; i++)
		{
			float value = data[i];
			element->QueryFloatAttribute(names[i], &value);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
		}
	}
}

void ProcessDrawItemDeferred(TiXmlElement *element, unsigned int id, std::vector<unsigned int> &buffer)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x974c9474 /* "pushmatrix" */:
		{
			buffer.push_back(0xf6604733 /* "glPushMatrix" */);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0xfc8a1d94 /* "glPopMatrix" */);
		}
		break;

	case 0x937cff81 /* "pushattrib" */:
		{
			GLuint mask = 0U;
			for (TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
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
			buffer.push_back(0xa471ec02 /* "glPushAttrib" */);
			buffer.push_back(mask);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x73c4cda1 /* "glPopAttrib" */);
		}
		break;

	case 0x052eb8b2 /* "pushclientattrib" */:
		{
			GLuint mask = 0U;
			for (TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
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
			buffer.push_back(0x485249b9 /* "glPushClientAttrib" */);
			buffer.push_back(mask);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0xbfd4add2 /* "glPopClientAttrib" */);
		}
		break;

	case 0xad0ecfd5 /* "translate" */:
		{
			float data[3] = { 0.0f, 0.0f, 0.0f };
			buffer.push_back(0xafeef11e /* "glTranslatef" */);
			ProcessDrawDataDeferred(element, id, buffer, 3, sPositionNames, data);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0x29e02ba1 /* "glRotatef" */);
			ProcessDrawDataDeferred(element, id, buffer, 4, sRotationNames, data);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			float data[3] = { 1.0f, 1.0f, 1.0f };
			buffer.push_back(0xff71cf6e /* "glScalef" */);
			ProcessDrawDataDeferred(element, id, buffer, 3, sPositionNames, data);
		}
		break;

	case 0x938fc4f7 /* "loadidentity" */:
		{
			buffer.push_back(0xcbd6bd5c /* "glLoadIdentity" */);
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
			buffer.push_back(0xca9090d7 /* "glLoadMatrixf" */);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				element->QueryFloatAttribute(name, &m[i]);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m[i]));
			}
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
			buffer.push_back(0x64500671 /* "glMultMatrixf" */);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				element->QueryFloatAttribute(name, &m[i]);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m[i]));
			}
		}
		break;

	case 0x945367a7 /* "vertex" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0x94110c7a /* "glVertex4f" */);
			ProcessDrawDataDeferred(element, id, buffer, 4, sPositionNames, data);
		}
		break;
	case 0xe68b9c52 /* "normal" */:
		{
			float data[3] = { 0.0f, 0.0f, 0.0f };
			buffer.push_back(0xf2d58094 /* "glNormal3f" */);
			ProcessDrawDataDeferred(element, id, buffer, 4, sPositionNames, data);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0x9d63d16b /* "glColor4f" */);
			ProcessDrawDataDeferred(element, id, buffer, 4, sColorNames, data);
		}
		break;

	case 0x090aa9ab /* "index" */:
		{
			float data[1] = { 0.0f };
			buffer.push_back(0xf3b3b82c /* "glIndexf" */);
			ProcessDrawDataDeferred(element, id, buffer, 1, sIndexNames, data);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			float data[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			buffer.push_back(0xb78bb2ae /* "glTexCoord4f" */);
			ProcessDrawDataDeferred(element, id, buffer, 4, sTexCoordNames, data);
		}
		break;

	case 0x0135ab46 /* "edgeflag" */:
		{
			int flag;
			if (element->QueryIntAttribute("flag", &flag) == TIXML_SUCCESS)
			{
				buffer.push_back(0x7f5dcd49 /* "glEdgeFlag" */);
				buffer.push_back(flag ? GL_TRUE : GL_FALSE);
			}
		}
		break;

	case 0x3c6468f4 /* "texture" */:
		break;

	case 0xbc9567c6 /* "points" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_POINTS);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_LINES);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_LINE_LOOP);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_LINE_STRIP);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xd8a57342 /* "triangles" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_TRIANGLES);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_TRIANGLE_STRIP);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_TRIANGLE_FAN);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_QUADS);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_QUAD_STRIP);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			buffer.push_back(0xb70e76e3 /* "glBegin" */);
			buffer.push_back(GL_POLYGON);
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer.push_back(0x50257afb /* "glEnd" */);
		}
		break;

	case 0xd2cf6b75 /* "calllist" */:
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				GLuint drawlist = Database::drawlist.Get(Hash(name));
				if (drawlist)
				{
					buffer.push_back(0x9525d6fe /* "glCallList" */);
					buffer.push_back(drawlist);
				}
			}
		}
		break;

	case 0x2610a4a3 /* "clientstate" */:
		{
			for (TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				switch (Hash(attrib->Name()))
				{
				case 0x945367a7 /* "vertex" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_VERTEX_ARRAY);
					break;
				case 0xe68b9c52 /* "normal" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_NORMAL_ARRAY);
					break;
				case 0x3d7e6258 /* "color" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_COLOR_ARRAY);
					break;
				case 0x090aa9ab /* "index" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_INDEX_ARRAY);
					break;
				case 0xdd612dd3 /* "texcoord" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_TEXTURE_COORD_ARRAY);
					break;
				case 0x0135ab46 /* "edgeflag" */:
					buffer.push_back(attrib->IntValue() ? 0x9128677b /* "glEnableClientState" */ : 0x342d0316 /* "glDisableClientState" */);
					buffer.push_back(GL_EDGE_FLAG_ARRAY);
					break;
				}
			}
		}
		break;

	case 0x6298bce4 /* "vertexarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
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

			buffer.push_back(0x4e467465 /* "glVertexPointer" */);
			buffer.push_back(size);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x81491d33 /* "normalarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
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

			buffer.push_back(0x46804012 /* "glNormalPointer" */);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0xcce5b995 /* "colorarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
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

			buffer.push_back(0x61e8560e /* "glColorPointer" */);
			buffer.push_back(size);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x664ead80 /* "indexarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
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

			buffer.push_back(0x1e5cf423 /* "glIndexPointer" */);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x91aa3148 /* "texcoordarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
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

			buffer.push_back(0x6d976421 /* "glTexCoordPointer" */);
			buffer.push_back(size);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (int i = 0; i < count; i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x60360ccf /* "edgeflagarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
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

			buffer.push_back(0x9cfbc596 /* "glEdgeFlagPointer" */);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (size_t i = 0; i < (count+sizeof(unsigned int)/sizeof(bool)-1)/(sizeof(unsigned int)/sizeof(bool)); i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i*sizeof(unsigned int)/sizeof(bool)]));
		}
		break;

	case 0x0a85bb5e /* "arrayelement" */:
		{
			int index;
			if (element->QueryIntAttribute("index", &index) == TIXML_SUCCESS)
			{
				buffer.push_back(0x8cfc8329 /* "glArrayElement" */);
				buffer.push_back(index);
			}
		}
		break;

	case 0xf4de4a21 /* "drawarrays" */:
		{
			GLenum mode;
			switch (Hash(element->Attribute("mode")))
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
			element->QueryIntAttribute("first", &first);
			element->QueryIntAttribute("count", &count);
			buffer.push_back(0x806f1b62 /* "glDrawArrays" */);
			buffer.push_back(mode);
			buffer.push_back(first);
			buffer.push_back(count);
		}
		break;

	case 0x757eeee2 /* "drawelements" */:
		{
			GLenum mode;
			switch (Hash(element->Attribute("mode")))
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

			const char *text = element->GetText();
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

			buffer.push_back(0xf6e885d9 /* "glDrawElements" */);
			buffer.push_back(mode);
			buffer.push_back(count);
			for (size_t i = 0; i < (count+sizeof(unsigned int)/sizeof(unsigned short)-1)/(sizeof(unsigned int)/sizeof(unsigned short)); i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&indices[i*sizeof(unsigned int)/sizeof(unsigned short)]));
		}
		break;

	case 0xd99ba82a /* "repeat" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);
			buffer.push_back(0xd99ba82a /* "repeat" */);
			buffer.push_back(count);
			buffer.push_back(0);
			int start = buffer.size();
			ProcessDrawItemsDeferred(element, id, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;

	default:
		break;
	}
}

void ProcessDrawItemsDeferred(TiXmlElement *element, unsigned int id, std::vector<unsigned int> &buffer)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ProcessDrawItemDeferred(child, id, buffer);
	}
}


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

void ProcessTemplateItem(TiXmlElement *element, unsigned int template_id)
{
	const char *value = element->Value();
	switch (Hash(value))
	{
	case 0x74e9dbae /* "collidable" */:
		{
			CollidableTemplate &collidable = Database::collidabletemplate.Open(template_id);
			collidable.Configure(element, template_id);
			Database::collidabletemplate.Close(template_id);
		}
		break;

	case 0x109dd1ad /* "renderable" */:
		{
			RenderableTemplate &renderable = Database::renderabletemplate.Open(template_id);
			renderable.Configure(element);
			Database::renderabletemplate.Close(template_id);
		}
		break;

	case 0x1b715375 /* "damagable" */:
		{
			DamagableTemplate &damagable = Database::damagabletemplate.Open(template_id);
			damagable.Configure(element);
			Database::damagabletemplate.Close(template_id);
		}
		break;

	case 0x0ddb0669 /* "link" */:
		{
			Database::Typed<LinkTemplate> &links = Database::linktemplate.Open(template_id);
			unsigned int sub_id = Hash(element->Attribute("name"));
			LinkTemplate &link = links.Open(sub_id);
			link.Configure(element);
			links.Close(sub_id);
			Database::linktemplate.Close(template_id);
		}
		break;

	case 0x2ea90881 /* "aimer" */:
		{
			AimerTemplate &aimer = Database::aimertemplate.Open(template_id);
			aimer.Configure(element);
			Database::aimertemplate.Close(template_id);
		}
		break;

	case 0xac56f17f /* "ship" */:
		{
			ShipTemplate &ship = Database::shiptemplate.Open(template_id);
			ship.Configure(element);
			Database::shiptemplate.Close(template_id);
		}
		break;

	case 0x6f332041 /* "weapon" */:
		{
			WeaponTemplate &weapon = Database::weapontemplate.Open(template_id);
			weapon.Configure(element);
			Database::weapontemplate.Close(template_id);
		}
		break;

	case 0xe894a379 /* "bullet" */:
		{
			BulletTemplate &bullet = Database::bullettemplate.Open(template_id);
			bullet.Configure(element);
			Database::bullettemplate.Close(template_id);
		}
		break;

	case 0x02bb1fe0 /* "explosion" */:
		{
			ExplosionTemplate &explosion = Database::explosiontemplate.Open(template_id);
			explosion.Configure(element, template_id);
			Database::explosiontemplate.Close(template_id);
		}
		break;

	case 0x4936726f /* "spawner" */:
		{
			SpawnerTemplate &spawner = Database::spawnertemplate.Open(template_id);
			spawner.Configure(element);
			Database::spawnertemplate.Close(template_id);
		}
		break;

	case 0xc98b019b /* "drawlist" */:
		{
			// create a new draw list
			GLuint handle = glGenLists(1);
			glNewList(handle, GL_COMPILE);

			// get the list name
			const char *name = element->Attribute("name");
			if (name)
			{
				// register the draw list
				Database::drawlist.Put(Hash(name), handle);
			}

			// process draw items
			ProcessDrawItems(element);

			// finish the draw list
			glEndList();
		}
		break;

	case 0x1ac6a97e /* "cloud" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);
			float mean = 256;
			element->QueryFloatAttribute("mean", &mean);
			float variance = 192;
			element->QueryFloatAttribute("variance", &variance);
			GLuint handle = CreateCloudDrawList(count, mean, variance);

			// get the list name
			const char *name = element->Attribute("name");
			if (name)
			{
				// register the draw list
				Database::drawlist.Put(Hash(name), handle);
			}
		}
		break;

	case 0xa2fd7d0c /* "team" */:
		{
			Database::team.Put(template_id, Hash(element->Attribute("name")));
		}
		break;
	}
}

void ProcessTemplateItems(TiXmlElement *element)
{
	// get template identifier
	const char *name = element->Attribute("name");
	unsigned int template_id = Hash(name);

	// get parent identifier
	const char *type = element->Attribute("type");
	unsigned int parent_id = Hash(type);

	// inherit parent components
	Database::Inherit(template_id, parent_id);

	// for each child element...
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		// process the template item
		ProcessTemplateItem(child, template_id);
	}

	Database::parent.Put(template_id, parent_id);
}

void ProcessEntityItems(TiXmlElement *element)
{
	// get entity identifier
	const char *name = element->Attribute("name");
	unsigned int entity_id = Hash(name);

	// get parent identifier
	const char *type = element->Attribute("type");
	unsigned int parent_id = Hash(type);

	// process template components
	ProcessTemplateItems(element);

	// create an entity
	Entity *entity = new Entity(entity_id);
	Database::entity.Put(entity_id, entity);

	// objects default to owning themselves
	Database::owner.Put(entity_id, entity_id);

	// process child elements
	// (components with no template equivalent)
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		if (entity->Configure(child))
			continue;

		switch (Hash(child->Value()))
		{
		case 0x2c99c300 /* "player" */:
			{
				Player *player = Database::player.Get(entity_id);
				if (!player)
				{
					player = new Player(entity_id);
					Database::controller.Put(entity_id, player);
					Database::player.Put(entity_id, player);
				}
				player->Configure(child);
			}
			break;

		case 0xe063cbaa /* "gunner" */:
			{
				Gunner *gunner = Database::gunner.Get(entity_id);
				if (!gunner)
				{
					gunner = new Gunner(entity_id);
					Database::gunner.Put(entity_id, gunner);
				}
				gunner->Configure(child);
			}
			break;
		}
	}
	
	// activate the instance
	// (create runtime components)
	Database::Activate(entity_id);
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
				// process template items
				ProcessTemplateItems(child);
			}
			break;

		case 0xd33ff5da /* "entity" */:
			{
				// process entity items
				ProcessEntityItems(child);
			}
			break;

		case 0x7f04120a /* "animateddrawlist" */:
			{
				// get the list name
				const char *name = child->Attribute("name");
				if (name)
				{
					std::vector<unsigned int> buffer;
					ProcessDrawItemsDeferred(child, Hash(name), buffer);
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
					Database::drawlist.Put(Hash(name), handle);
				}

				// process draw items
				ProcessDrawItems(child);

				// finish the draw list
				glEndList();
			}
			break;

		case 0x1ac6a97e /* "cloud" */:
			{
				int count = 1;
				child->QueryIntAttribute("count", &count);
				float mean = 256;
				child->QueryFloatAttribute("mean", &mean);
				float variance = 192;
				child->QueryFloatAttribute("variance", &variance);
				GLuint handle = CreateCloudDrawList(count, mean, variance);

				// get the list name
				const char *name = child->Attribute("name");
				if (name)
				{
					// register the draw list
					Database::drawlist.Put(Hash(name), handle);
				}
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
		if (argv[i][0] == '-' || argv[i][0] == '/')
		{
			switch (Hash(argv[i]+1))
			{
			case 0x1d215c8f /* "resolution" */:
				SCREEN_WIDTH = atoi(argv[++i]);
				SCREEN_HEIGHT = atoi(argv[++i]);
				break;

			case 0xfe759eea /* "depth" */:
				SCREEN_DEPTH = atoi(argv[++i]);
				break;

			case 0x5032fb58 /* "fullscreen" */:
				SCREEN_FULLSCREEN = atoi(argv[++i]) != 0;
				break;

			case 0x06f8f066 /* "vsync" */:
				OPENGL_SWAPCONTROL = atoi(argv[++i]) != 0;
				break;

			case 0x35c8978f /* "antialias" */:
				OPENGL_ANTIALIAS = atoi(argv[++i]) != 0;
				break;

			case 0x47d0f228 /* "multisample" */:
				OPENGL_MULTISAMPLE = atoi(argv[++i]);
				break;

			case 0x68b9bf22 /* "doublebuffer" */:
				OPENGL_DOUBLEBUFFER = atoi(argv[++i]) != 0;
				break;

			case 0xcc87a64d /* "stereo" */:
				OPENGL_STEREO = atoi(argv[++i]) != 0;
				break;

			case 0xb5708afc /* "accelerated" */:
				OPENGL_ACCELERATED = atoi(argv[++i]) != 0;
				break;

			case 0xf9d86f7b /* "input" */:
				inputconfig = argv[++i];
				break;

			case 0x9b99e7dd /* "level" */:
				levelconfig = argv[++i];
				break;

			case 0xd6974b06 /* "simrate" */:
				SIMULATION_RATE = atoi(argv[++i]);
				break;

			case 0x9f2f269e /* "timescale" */:
				TIME_SCALE = float(atof(argv[++i]));
				break;
			}
		}
	}

	// quit flag
	bool quit = false;

	// initialize
	if( !init() )
		return 1;    

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

    // timer timer
    Timer timer;
    
    // start timer
    timer.start();

	// last ticks
	int ticks = timer.get_ticks();

	// simulation timer
	const float sim_rate = float(SIMULATION_RATE);
	const float sim_step = 1.0f / sim_rate;
	float sim_timer = 1.0f;

	// camera track position
	Vector2 trackpos(0, 0);

	// wait for user exit
	do
	{
		// INPUT PHASE

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
		if (delta > 100)
			delta = 100;

		// advance the sim timer
		sim_timer += delta * TIME_SCALE * sim_rate / 1000.0f;

		// while simulation turns to run...
		while (sim_timer > 1.0f)
		{
			// deduct a turn
			sim_timer -= 1.0f;
			
			// update database
			Database::Update();


			// CONTROL PHASE

#ifdef PRINT_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_freq;
			QueryPerformanceFrequency(&perf_freq);

			LARGE_INTEGER perf_count0;
			QueryPerformanceCounter(&perf_count0);
#endif

			// control all entities
			Controller::ControlAll(sim_step);

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


			// update inputs for next step
			input.Update();
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

		// get the player ship
		const Entity *player = Database::entity.Get(0xeec1dafa /* "playership" */);
		if (player)
		{
			// track player position
			trackpos = player->GetInterpolatedPosition(sim_timer);
		}

		// draw player health (HACK)
		Damagable *damagable = Database::damagable.Get(0xeec1dafa /* "playership" */);
		if (damagable)
		{
			float health = damagable->GetHealth();

			// push camera transform
			glPushMatrix();

			// use 640x480 screen coordinates
			glLoadIdentity();
			glScalef( 1.0f / 640, 1.0f / 640, -1.0f );
			glTranslatef(-0.5f*640, -0.5f*640*SCREEN_HEIGHT/SCREEN_WIDTH, 1.0f);

			glBegin(GL_QUADS);

			// set color based on health
			if (health < 5)
				glColor4f(1.0f, 0.1f + (health - 1) * 0.9f / 4, 0.1f, 1.0f - health * 0.09f);
			else if (health < 10)
				glColor4f(0.1f + (10 - health) * 0.9f / 5, 1.0f, 0.1f, 1.0f - health * 0.09f);
			else
				glColor4f(0.1f, 1.0f, 0.1f, 0.1f);

			// fill gauge
			glVertex2f(8, 8);
			glVertex2f(8 + 8 * health, 8);
			glVertex2f(8 + 8 * health, 16);
			glVertex2f(8, 16);

			// background
			glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
			glVertex2f(8 + 8 * health, 8);
			glVertex2f(88, 8);
			glVertex2f(88, 16);
			glVertex2f(8 + 8 * health, 16);

			glEnd();

			// reset camera transform
			glPopMatrix();
		}

		// push camera transform
		glPushMatrix();

		// set camera to track position
		glTranslatef( -trackpos.x, -trackpos.y, 0 );

		// render all entities
		// (send interpolation ratio and offset from simulation time)
		Renderable::RenderAll(sim_timer, sim_step);

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

	// remove all entities
//	for (Database::Typed<Entity *>::iterator itor = Database::entity.begin(); itor != Database::entity.end(); ++itor)
//		delete itor->second;
	Database::entity.Clear();

	// remove all drawlists
	Database::drawlist.Clear();

	// clear databases
	Database::player.Clear();
	Database::gunner.Clear();
	Database::collidable.Clear();
	Database::collidabletemplate.Clear();
	Database::renderable.Clear();
	Database::renderabletemplate.Clear();
	Database::bullettemplate.Clear();
	Database::explosiontemplate.Clear();

	// collidable done
	Collidable::WorldDone();

	// clean up
	clean_up();

	// done
	return 0;
}
