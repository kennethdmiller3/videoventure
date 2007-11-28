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
typedef stdext::hash_map<unsigned int, Entity *> EntityMap;
EntityMap entities;

// drawlist map
typedef stdext::hash_map<unsigned int, unsigned int> DrawListMap;
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

static void ProcessDrawItems(TiXmlElement *element)
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
						if (atoi(attrib->Value()))
							mask |= GL_CURRENT_BIT;
						else
							mask &= ~GL_CURRENT_BIT;
						break;
					case 0x18ae6c91 /* "point" */:
						if (atoi(attrib->Value()))
							mask |= GL_POINT_BIT;
						else
							mask &= ~GL_POINT_BIT;
						break;
					case 0x17db1627 /* "line" */:
						if (atoi(attrib->Value()))
							mask |= GL_LINE_BIT;
						else
							mask &= ~GL_LINE_BIT;
						break;
					case 0x051cb889 /* "polygon" */:
						if (atoi(attrib->Value()))
							mask |= GL_POLYGON_BIT;
						else
							mask &= ~GL_POLYGON_BIT;
						break;
					case 0x67b14997 /* "polygon_stipple" */:
						if (atoi(attrib->Value()))
							mask |= GL_POLYGON_STIPPLE_BIT;
						else
							mask &= ~GL_POLYGON_STIPPLE_BIT;
						break;
					case 0xccde91eb /* "pixel_mode" */:
						if (atoi(attrib->Value()))
							mask |= GL_LIGHTING_BIT;
						else
							mask &= ~GL_LIGHTING_BIT;
						break;
					case 0x827eb1c9 /* "lighting" */:
						if (atoi(attrib->Value()))
							mask |= GL_POINT_BIT;
						else
							mask &= ~GL_POINT_BIT;
						break;
					case 0xa1f3723f /* "fog" */:
						if (atoi(attrib->Value()))
							mask |= GL_FOG_BIT;
						else
							mask &= ~GL_FOG_BIT;
						break;
					case 0x65e5b825 /* "depth_buffer" */:
						if (atoi(attrib->Value()))
							mask |= GL_DEPTH_BUFFER_BIT;
						else
							mask &= ~GL_DEPTH_BUFFER_BIT;
						break;
					case 0x907f6213 /* "accum_buffer" */:
						if (atoi(attrib->Value()))
							mask |= GL_ACCUM_BUFFER_BIT;
						else
							mask &= ~GL_ACCUM_BUFFER_BIT;
						break;
					case 0x632020be /* "stencil_buffer" */:
						if (atoi(attrib->Value()))
							mask |= GL_STENCIL_BUFFER_BIT;
						else
							mask &= ~GL_STENCIL_BUFFER_BIT;
						break;
					case 0xe4abbac3 /* "viewport" */:
						if (atoi(attrib->Value()))
							mask |= GL_VIEWPORT_BIT;
						else
							mask &= ~GL_VIEWPORT_BIT;
						break;
					case 0xe1ad931b /* "transform" */:
						if (atoi(attrib->Value()))
							mask |= GL_TRANSFORM_BIT;
						else
							mask &= ~GL_TRANSFORM_BIT;
						break;
					case 0xaf8bb8ce /* "enable" */:
						if (atoi(attrib->Value()))
							mask |= GL_ENABLE_BIT;
						else
							mask &= ~GL_ENABLE_BIT;
						break;
					case 0x0d759bbb /* "color_buffer" */:
						if (atoi(attrib->Value()))
							mask |= GL_COLOR_BUFFER_BIT;
						else
							mask &= ~GL_COLOR_BUFFER_BIT;
						break;
					case 0x4bc809b8 /* "hint" */:
						if (atoi(attrib->Value()))
							mask |= GL_HINT_BIT;
						else
							mask &= ~GL_HINT_BIT;
						break;
					case 0x08d22e0f /* "eval" */:
						if (atoi(attrib->Value()))
							mask |= GL_EVAL_BIT;
						else
							mask &= ~GL_EVAL_BIT;
						break;
					case 0x0cfb5881 /* "list" */:
						if (atoi(attrib->Value()))
							mask |= GL_LIST_BIT;
						else
							mask &= ~GL_LIST_BIT;
						break;
					case 0x3c6468f4 /* "texture" */:
						if (atoi(attrib->Value()))
							mask |= GL_TEXTURE_BIT;
						else
							mask &= ~GL_TEXTURE_BIT;
						break;
					case 0x0adbc081 /* "scissor" */:
						if (atoi(attrib->Value()))
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

		case 0xad0ecfd5 /* "translate" */:
			{
				const char *x = child->Attribute("x");
				const char *y = child->Attribute("y");
				const char *z = child->Attribute("z");
				glTranslatef(x ? float(atof(x)) : 0, y ? float(atof(y)) : 0, z ? float(atof(z)) : 0);
			}
			break;

		case 0xa5f4fd0a /* "rotate" */:
			{
				const char *a = child->Attribute("angle");
				const char *x = child->Attribute("x");
				const char *y = child->Attribute("y");
				const char *z = child->Attribute("z");
				glRotatef(a ? float(atof(a)) : 0, x ? float(atof(x)) : 0, y ? float(atof(y)) : 0, z ? float(atof(z)) : 0);
			}
			break;

		case 0x82971c71 /* "scale" */:
			{
				const char *x = child->Attribute("x");
				const char *y = child->Attribute("y");
				const char *z = child->Attribute("z");
				glScalef(x ? float(atof(x)) : 0, y ? float(atof(y)) : 0, z ? float(atof(z)) : 0);
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
					const char *value = child->Attribute(name);
					if (value)
						m[i] = float(atof(value));					
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
					const char *value = child->Attribute(name);
					if (value)
						m[i] = float(atof(value));					
				}
				glMultMatrixf(m);
			}
			break;

		case 0x3d7e6258 /* "color" */:
			{
				const char *r = child->Attribute("r");
				const char *g = child->Attribute("g");
				const char *b = child->Attribute("b");
				const char *a = child->Attribute("a");
				glColor4d(r ? float(atof(r)) : 0, g ? float(atof(g)) : 0, b ? float(atof(b)) : 0, a ? float(atof(a)) : 1);
			}
			break;

		case 0x945367a7 /* "vertex" */:
			{
				const char *x = child->Attribute("x");
				const char *y = child->Attribute("y");
				const char *z = child->Attribute("z");
				const char *w = child->Attribute("w");
				glVertex4d(x ? float(atof(x)) : 0, y ? float(atof(y)) : 0, z ? float(atof(z)) : 0, w ? float(atof(w)) : 1);
			}
			break;

		case 0xdd612dd3 /* "texcoord" */:
			{
				const char *s = child->Attribute("s");
				const char *t = child->Attribute("t");
				const char *r = child->Attribute("r");
				const char *q = child->Attribute("q");
				glTexCoord4d(s ? float(atof(s)) : 0, t ? float(atof(t)) : 0, r ? float(atof(r)) : 0, q ? float(atof(q)) : 1);
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

		default:
			break;
		}
	}
}

static void ProcessRenderableItems(TiXmlElement *element, Renderable *renderable)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xc98b019b /* "drawlist" */:
			{
				// get the list name
				const char *name = child->Attribute("name");
				if (name)
				{
					// find the named drawlist
					DrawListMap::iterator itor = drawlists.find(Hash(name));
					if (itor != drawlists.end())
					{
						// use the named drawlist
						renderable->mDraw = itor->second;
						break;
					}
				}

				if (child->FirstChildElement())
				{
					// create a new draw list
					GLuint handle = glGenLists(1);
					glNewList(handle, GL_COMPILE);

					// process draw items
					ProcessDrawItems(child);

					// finish the draw list
					glEndList();

					// use the anonymous drawlist
					renderable->mDraw = handle;
					break;
				}
			}
			break;

		default:
			break;
		}
	}
}

static void ProcessCollidableItems(TiXmlElement *element, Collidable *collidable)
{
	// process child elements
	for (TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
	{
		const char *label = attrib->Name();
		const char *value = attrib->Value();
		switch (Hash(label))
		{
		case 0x07a640f6 /* "layer" */:
			{
				collidable->SetLayer(value ? atoi(value) : -1);
			}
			break;

		case 0x5127f14d /* "type" */:
			{
				switch (Hash(value))
				{
				case 0x06dbc8c0 /* "alignedbox" */:
					collidable->type = Collidable::TYPE_ALIGNED_BOX;
					break;
				case 0x28217089 /* "circle" */:
					collidable->type = Collidable::TYPE_CIRCLE;
					break;
				default:
					collidable->type = Collidable::TYPE_NONE;
					break;
				}
			}
			break;

		case 0x0dba4cb3 /* "radius" */:
			{
				collidable->size.x = value ? float(atof(value)) : 0;
				collidable->size.y = value ? float(atof(value)) : 0;
			}
			break;

		case 0x95876e1f /* "width" */:
			{
				collidable->size.x = value ? float(atof(value)) : 0;
			}
			break;

		case 0xd5bdbb42 /* "height" */:
			{
				collidable->size.y = value ? float(atof(value)) : 0;
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
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x934f4e0a /* "position" */:
			{
				const char *x = child->Attribute("x");
				const char *y = child->Attribute("y");
				Vector2 pos(x ? float(atof(x)) : 0, y ? float(atof(y)) : 0);
				entity->SetPosition(pos);
			}
			break;

		case 0x32741c32 /* "velocity" */:
			{
				const char *x = child->Attribute("x");
				const char *y = child->Attribute("y");
				Vector2 vel(x ? float(atof(x)) : 0, y ? float(atof(y)) : 0);
				entity->SetVelocity(vel);
			}
			break;

		case 0x74e9dbae /* "collidable" */:
			{
				Collidable *collidable = dynamic_cast<Collidable *>(entity);
				if (collidable)
				{
					ProcessCollidableItems(child, collidable);
				}
			}
			break;

		case 0x109dd1ad /* "renderable" */:
			{
				Renderable *renderable = dynamic_cast<Renderable *>(entity);
				if (renderable)
				{
					ProcessRenderableItems(child, renderable);
				}
			}
			break;

		default:
			break;
		}
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
						const char *count = child->Attribute("count");
						entity = new Cloud(count ? atoi(count) : 1);
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

	// input binding
	Input input;
	input.Bind(Input::MOVE_UP, SDLK_w);
	input.Bind(Input::MOVE_DOWN, SDLK_s);
	input.Bind(Input::MOVE_LEFT, SDLK_a);
	input.Bind(Input::MOVE_RIGHT, SDLK_d);
	input.Bind(Input::FIRE_PRIMARY, SDL_BUTTON_LEFT);

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

	// find the gunners
	Gunner *gunner[2] = { NULL, NULL };
	{
		EntityMap::iterator itor = entities.find(Hash("gunner1"));
		if (itor != entities.end())
		{
			gunner[0] = dynamic_cast<Gunner *>(itor->second);
			gunner[0]->SetPlayer(player);
			gunner[0]->SetInput(&input);
			gunner[0]->SetPhase(0);
		}
	}
	{
		EntityMap::iterator itor = entities.find(Hash("gunner2"));
		if (itor != entities.end())
		{
			gunner[1] = dynamic_cast<Gunner *>(itor->second);
			gunner[1]->SetPlayer(player);
			gunner[1]->SetInput(&input);
			gunner[1]->SetPhase(1);
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
				input.OnKeyDown( event.key.keysym.sym );
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
				input.OnKeyUp( event.key.keysym.sym );
				break;
			case SDL_MOUSEBUTTONDOWN:
				input.OnKeyDown( event.button.button );
				break;
			case SDL_MOUSEBUTTONUP:
				input.OnKeyUp( event.button.button );
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
