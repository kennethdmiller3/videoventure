// precompiled header
#include "stdafx.h"

// includes
#include "oglconsole.h"                                                                              
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
#include "Drawlist.h"

// screen attributes
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
int SCREEN_DEPTH = 0;
bool SCREEN_FULLSCREEN = false;

// view attributes
float VIEW_SIZE = 320;
float VIEW_AIM = 100;
float VIEW_AIM_FILTER = 2.0f;

// opengl attributes
bool OPENGL_DOUBLEBUFFER = true;
bool OPENGL_STEREO = false;
bool OPENGL_ACCELERATED = true;
bool OPENGL_SWAPCONTROL = true;
bool OPENGL_ANTIALIAS = false;
int OPENGL_MULTISAMPLE = 16;


// debug output
bool DEBUGPRINT_OUTPUTCONSOLE = false;
bool DEBUGPRINT_OUTPUTDEBUG = true;

// simulation attributes
int SIMULATION_RATE = 60;
float TIME_SCALE = 1.0f;

// rendering attributes
int RENDER_MOTIONBLUR = 1;


// default input configuration
const char *INPUT_CONFIG = "input.xml";

// default level configuration
const char *LEVEL_CONFIG = "level.xml";


// console
OGLCONSOLE_Console console;

//#define PRINT_PERFORMANCE_DETAILS
//#define PRINT_SIMULATION_TIMER
#define TRACE_OPENGL_ATTRIBUTES

// input system
Input input;

// forward declaration
int ProcessCommand( unsigned int aCommand, char *aParam[] );

// debug output
int DebugPrint(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
#ifdef WIN32
	char buf[4096];
	int n = vsnprintf(buf, sizeof(buf), format, ap);
	if (DEBUGPRINT_OUTPUTDEBUG)
		OutputDebugStringA(buf);
	if (DEBUGPRINT_OUTPUTCONSOLE && console)
		OGLCONSOLE_Output(console, "%s", buf);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

void cmdCB(OGLCONSOLE_Console console, char *cmd)
{
	// copy the command string
	char buf[256];
	strncpy(buf, cmd, sizeof(buf)-1);
	buf[sizeof(buf)-1] = '\0';

	// get the command
	char *token = strtok(buf, " \t");
	unsigned int command = Hash(token);

	// parameter list
	char *param[64] = { 0 };
	int count = 0;

	// get parameters
	token = strtok(NULL, " \t");
	while (token)
	{
		param[count++] = token;
		token = strtok(NULL, " \t");
	}

	// process the command
	ProcessCommand(command, param);
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
	glScalef( -1.0f / VIEW_SIZE, -1.0f / VIEW_SIZE, -1.0f );
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

	// set window title
	SDL_WM_SetCaption( "Shmup!", NULL );

    /* Initialize OGLCONSOLE */                                                                      
    console = OGLCONSOLE_Create();                                                                             
    OGLCONSOLE_EnterKey(cmdCB);                                                                      

	// initialize OpenGL
	if( !init_GL() )
		return false;    

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
    /* clean up oglconsole */                                                                        
    OGLCONSOLE_Quit();

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


void ProcessTemplateItem(const TiXmlElement *element, unsigned int template_id)
{
	const char *value = element->Value();
	const Database::Loader::Entry &configure = Database::Loader::GetConfigure(Hash(value));
	if (configure)
		configure(template_id, element);
	else
		DebugPrint("Unrecognized tag \"%s\"\n", value);
}

void ProcessTemplateItems(const TiXmlElement *element)
{
	// get template identifier
	const char *name = element->Attribute("name");
	unsigned int template_id = Hash(name);

	// get parent identifier
	const char *type = element->Attribute("type");
	unsigned int parent_id = Hash(type);

	// inherit parent components
	Database::Inherit(template_id, parent_id);

	// set name
	std::string &namebuf = Database::name.Open(template_id);
	namebuf = name;
	Database::name.Close(template_id);

	// for each child element...
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		// process the template item
		ProcessTemplateItem(child, template_id);
	}

	// set parent
//	Database::parent.Put(template_id, parent_id);
}

void ProcessEntityItems(const TiXmlElement *element)
{
	// get entity identifier
	const char *name = element->Attribute("name");
	unsigned int entity_id = Hash(name);

	// get parent identifier
	const char *type = element->Attribute("type");
	unsigned int parent_id = Hash(type);

	// inherit components from template
//	Database::Inherit(entity_id, parent_id);

	// set name
	std::string &namebuf = Database::name.Open(entity_id);
	namebuf = name;
	Database::name.Close(entity_id);
	
	// set parent
	Database::parent.Put(entity_id, parent_id);

	// objects default to owning themselves
	Database::owner.Put(entity_id, entity_id);

	// create an entity
	Entity *entity = new Entity(entity_id);
	Database::entity.Put(entity_id, entity);

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		if (entity->Configure(child))
			continue;

		// process the template item
		ProcessTemplateItem(child, entity_id);
	}

	// activate the instance
	// (create runtime components)
	Database::Activate(entity_id);
}

static void ProcessWorldItems(const TiXmlElement *element)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *value = child->Value();
		DebugPrint("Processing %s (%s)\n", child->Value(), child->Attribute("name"));
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
					ProcessDrawItems(child, buffer);
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

				// get (optional) parameter value
				float param = 0.0f;
				child->QueryFloatAttribute("param", &param);

				// process draw items
				std::vector<unsigned int> drawlist;
				ProcessDrawItems(child, drawlist);
				ExecuteDrawItems(&drawlist[0], drawlist.size(), param);

				// finish the draw list
				glEndList();
			}
			break;

		case 0x3c6468f4 /* "texture" */:
			if (const char *file = child->Attribute("file"))
			{
				if (SDL_Surface *surface = SDL_LoadBMP(file))
				{ 	 
					// check if the width is a power of 2
					if ((surface->w & (surface->w - 1)) != 0)
					{
						DebugPrint("warning: %s width %d is not a power of 2\n", file, surface->w);
					}
					
					// check if the height is a power of 2
					if ((surface->h & (surface->h - 1)) != 0)
					{
						DebugPrint("warning: %s height %d is not a power of 2\n", file, surface->h);
					}
				 
					// get the number of channels in the SDL surface
					GLenum texture_format;
					GLint color_size = surface->format->BytesPerPixel;
					if (color_size == 4)     // contains an alpha channel
					{
						if (surface->format->Rmask == 0x000000ff)
							texture_format = GL_RGBA;
						else
							texture_format = GL_BGRA;
					}
					else if (color_size == 3)     // no alpha channel
					{
						if (surface->format->Rmask == 0x000000ff)
							texture_format = GL_RGB;
						else
							texture_format = GL_BGR;
					}
					else
					{
						DebugPrint("warning: %s is not truecolor\n", file);
						SDL_FreeSurface( surface );
						break;
					}

					// generate a texture object handle
					GLuint texture;
					glGenTextures( 1, &texture );

					// get the list name
					const char *name = child->Attribute("name");
					if (name)
					{
						// register the draw list
						Database::texture.Put(Hash(name), texture);
					}

					// save texture state
					glPushAttrib(GL_TEXTURE_BIT);

					// bind the texture object
					glBindTexture(GL_TEXTURE_2D, texture);

					// mode
					GLint mode;

					// set blend mode
					switch (Hash(child->Attribute("mode")))
					{
					default:
					case 0x818f75ae /* "modulate" */:	mode = GL_MODULATE; break;
					case 0xde15f6ae /* "decal" */:		mode = GL_DECAL; break;
					case 0x0bbc40d8 /* "blend" */:		mode = GL_BLEND; break;
					case 0xa13884c3 /* "replace" */:	mode = GL_REPLACE; break;
					}
					glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );

					// set minification filter
					switch (Hash(child->Attribute("minfilter")))
					{
					default:
					case 0xc42bfa19 /* "nearest" */:			mode = GL_NEAREST; break;
					case 0xd00594c0 /* "linear" */:				mode = GL_LINEAR; break;
					case 0x70bf16c1 /* "nearestmipnearest" */:	mode = GL_NEAREST_MIPMAP_NEAREST; break;
					case 0xc81505e8 /* "linearmipnearest" */:	mode = GL_LINEAR_MIPMAP_NEAREST; break;
					case 0x95d62f98 /* "nearestmiplinear" */:	mode = GL_NEAREST_MIPMAP_LINEAR; break;
					case 0x1274a447 /* "linearmiplinear" */:	mode = GL_LINEAR_MIPMAP_LINEAR; break;
					}
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode );

					// set magnification filter
					switch (Hash(child->Attribute("magfilter")))
					{
					default:
					case 0xc42bfa19 /* "nearest" */:			mode = GL_NEAREST; break;
					case 0xd00594c0 /* "linear" */:				mode = GL_LINEAR; break;
					}
				    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode );

					// set s wrapping
					switch (Hash(child->Attribute("wraps")))
					{
					default:
					case 0xa82efcbc /* "clamp" */:				mode = GL_CLAMP; break;
					case 0xd99ba82a /* "repeat" */:				mode = GL_REPEAT; break;
					}
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode );

					// set t wrapping
					switch (Hash(child->Attribute("wrapt")))
					{
					default:
					case 0xa82efcbc /* "clamp" */:				mode = GL_CLAMP; break;
					case 0xd99ba82a /* "repeat" */:				mode = GL_REPEAT; break;
					}
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode );

					// set texture image data
					glTexImage2D(
						GL_TEXTURE_2D, 0, color_size, surface->w, surface->h, 0,
						texture_format, GL_UNSIGNED_BYTE, surface->pixels
						);

					// restore texture state
					glPopAttrib();

					// free the surface
					SDL_FreeSurface( surface );
				}
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


// commands
int ProcessCommand( unsigned int aCommand, char *aParam[] )
{
	switch (aCommand)
	{
	case 0x1d215c8f /* "resolution" */:
		SCREEN_WIDTH = atoi(aParam[0]);
		SCREEN_HEIGHT = atoi(aParam[1]);
		return 2;

	case 0xfe759eea /* "depth" */:
		SCREEN_DEPTH = atoi(aParam[0]);
		return 1;

	case 0x5032fb58 /* "fullscreen" */:
		SCREEN_FULLSCREEN = atoi(aParam[0]) != 0;
		return 1;

	case 0x06f8f066 /* "vsync" */:
		OPENGL_SWAPCONTROL = atoi(aParam[0]) != 0;
		return 1;

	case 0x35c8978f /* "antialias" */:
		OPENGL_ANTIALIAS = atoi(aParam[0]) != 0;
		return 1;

	case 0x47d0f228 /* "multisample" */:
		OPENGL_MULTISAMPLE = atoi(aParam[0]);
		return 1;

	case 0x68b9bf22 /* "doublebuffer" */:
		OPENGL_DOUBLEBUFFER = atoi(aParam[0]) != 0;
		return 1;

	case 0xcc87a64d /* "stereo" */:
		OPENGL_STEREO = atoi(aParam[0]) != 0;
		return 1;

	case 0xb5708afc /* "accelerated" */:
		OPENGL_ACCELERATED = atoi(aParam[0]) != 0;
		return 1;

	case 0x1ae79789 /* "viewsize" */:
		VIEW_SIZE = float(atof(aParam[0]));
		return 1;

	case 0x8e6b4341 /* "viewaim" */:
		VIEW_AIM = float(atof(aParam[0]));
		return 1;

	case 0xd49cb7d3 /* "viewaimfilter" */:
		VIEW_AIM_FILTER = float(atof(aParam[0]));
		return 1;

	case 0xf9d86f7b /* "input" */:
		INPUT_CONFIG = aParam[0];
		return 1;

	case 0x9b99e7dd /* "level" */:
		LEVEL_CONFIG = aParam[0];
		return 1;

	case 0xd6974b06 /* "simrate" */:
		SIMULATION_RATE = atoi(aParam[0]);
		return 1;

	case 0x9f2f269e /* "timescale" */:
		TIME_SCALE = float(atof(aParam[0]));
		return 1;

	case 0xf744f3b2 /* "motionblur" */:
		RENDER_MOTIONBLUR = atoi(aParam[0]);
		return 1;

	case 0x94c716fd /* "outputconsole" */:
		DEBUGPRINT_OUTPUTCONSOLE = atoi(aParam[0]) != 0;
		return 1;

	case 0x54822903 /* "outputdebug" */:
		DEBUGPRINT_OUTPUTDEBUG = atoi(aParam[0]) != 0;
		return 1;

	case 0xa165ddb8 /* "database" */:
		{
			unsigned int id = Hash(aParam[0]);
			Database::Untyped *db = Database::GetDatabases().Get(id);
			if (db)
			{
				OGLCONSOLE_Output(console, "stride=%d shift=%d bits=%d limit=%d count=%d\n",
					db->GetStride(), db->GetShift(), db->GetBits(), db->GetLimit(), db->GetCount());
			}
			else
			{
				OGLCONSOLE_Output(console, "database \"%s\" (0x%08x) not found\n", aParam[0], id);
			}
		}
		return 1;

	case 0xbdf0855a /* "find" */:
		{
			unsigned int id = Hash(aParam[0]);
			Database::Untyped *db = Database::GetDatabases().Get(id);
			if (db)
			{
				unsigned int key = Hash(aParam[1]);
				if (const void *data = db->Find(key))
				{
					OGLCONSOLE_Output(console, "record \"%s\" (0x%08x) data=0x%p\n", aParam[1], key, data);
				}
				else
				{
					OGLCONSOLE_Output(console, "record \"%s\" (0x%08x) not found\n", aParam[1], key);
				}
			}
			else
			{
				OGLCONSOLE_Output(console, "database \"%s\" (0x%08x) not found\n", aParam[0], id);
			}
		}
		return 2;

	case 0x0cfb5881 /* "list" */:
		{
			unsigned int id = Hash(aParam[0]);
			Database::Untyped *db = Database::GetDatabases().Get(id);
			if (db)
			{
				for (Database::Untyped::Iterator itor(db); itor.IsValid(); ++itor)
				{
					OGLCONSOLE_Output(console, "%d: name=\"%s\" key=0x%08x data=0x%p\n", itor.GetSlot(), Database::name.Get(itor.GetKey()).c_str(), itor.GetKey(), itor.GetValue());
				}
			}
		}
		return 1;

	default:
		return 0;
	}
}

// main
int SDL_main( int argc, char *argv[] )
{
	// process command-line arguments
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-' || argv[i][0] == '/')
		{
			i += ProcessCommand(Hash(argv[i]+1), argv+i+1);
		}
	}

	// quit flag
	bool quit = false;

	// initialize
	if( !init() )
		return 1;    

	// clear the screen
	glClear(
		GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_BUFFER
		| GL_DEPTH_BUFFER_BIT
#endif
		);
	// show the screen
	SDL_GL_SwapBuffers();

	{
		// input binding
		DebugPrint("Input %s\n", INPUT_CONFIG);
		TiXmlDocument document(INPUT_CONFIG);
		document.LoadFile();

		TiXmlHandle handle( &document );
		TiXmlElement *element = handle.FirstChildElement("input").ToElement();
		if (element)
		{
			for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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
		DebugPrint("Level %s\n", LEVEL_CONFIG);
		TiXmlDocument document(LEVEL_CONFIG);
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
	unsigned int sim_turn = 0;

	DebugPrint("Simulating at %dHz (x%f)\n", SIMULATION_RATE, TIME_SCALE);

	// camera track position
	Vector2 trackpos(0, 0);
	Vector2 trackaim(0, 0);

	// wait for user exit
	do
	{
		// INPUT PHASE

		// event handler
		SDL_Event event;

		// process events
		while( SDL_PollEvent( &event ) )
		{
			/* Give the console first dibs on event processing */                                    
            if (OGLCONSOLE_SDLEvent(&event))
				continue;

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
				input.OnAxis( INPUT_TYPE_MOUSE_AXIS, event.motion.which, 0, float(event.motion.x * 2 - SCREEN_WIDTH) / float(SCREEN_HEIGHT) );
				input.OnAxis( INPUT_TYPE_MOUSE_AXIS, event.motion.which, 1, float(event.motion.y * 2 - SCREEN_HEIGHT) / float(SCREEN_HEIGHT) );
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

		// delta time
		float delta_time = delta * TIME_SCALE / 1000.0f / RENDER_MOTIONBLUR;

		// delta step
		float delta_step = delta_time * sim_rate;

		// for each motion-blur step
		for (int blur = 0; blur < RENDER_MOTIONBLUR; ++blur)
		{
			// advance the sim timer
			sim_timer += delta_step;

			// while simulation turns to run...
			while (sim_timer >= 1.0f)
			{
				// deduct a turn
				sim_timer -= 1.0f;
				
				// update database
				Database::Update();

				// update input values
				input.Update();


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

				DebugPrint("C=%d ", 1000000 * (perf_count1.QuadPart - perf_count0.QuadPart) / perf_freq.QuadPart);
#endif

				// SIMULATION PHASE
				// (generate forces)
				Simulatable::SimulateAll(sim_step);

#ifdef PRINT_PERFORMANCE_DETAILS
				LARGE_INTEGER perf_count2;
				QueryPerformanceCounter(&perf_count2);

				DebugPrint("S=%d ", 1000000 * (perf_count2.QuadPart - perf_count1.QuadPart) / perf_freq.QuadPart);
#endif

				// COLLISION PHASE
				// (apply forces and update positions)
				Collidable::CollideAll(sim_step);

#ifdef PRINT_PERFORMANCE_DETAILS
				LARGE_INTEGER perf_count3;
				QueryPerformanceCounter(&perf_count3);

				DebugPrint("P=%d ", 1000000 * (perf_count3.QuadPart - perf_count2.QuadPart) / perf_freq.QuadPart);
#endif

				// UPDATE PHASE
				// (use updated positions)
				Updatable::UpdateAll(sim_step);

				// step inputs for next turn
				input.Step();

				// advance the turn counter
				++sim_turn;
				Renderable::SetTurn(sim_turn);
			}

#ifdef PRINT_SIMULATION_TIMER
			DebugPrint("delta=%d ticks=%d sim_t=%f\n", delta, ticks, sim_timer);
#endif

			// RENDERING PHASE

			// for each player...
			for (Database::Typed<Player *>::Iterator itor(&Database::player); itor.IsValid(); ++itor)
			{
				// get the entity
				Entity *entity = Database::entity.Get(itor.GetKey());

				// track player position
				trackpos = entity->GetInterpolatedPosition(sim_timer);

				// if applying view aim
				if (VIEW_AIM)
				{
					if (Database::ship.Get(itor.GetKey()))
						trackaim += VIEW_AIM_FILTER * delta_time * (itor.GetValue()->mAim - trackaim);
					else
						trackaim -= VIEW_AIM_FILTER * delta_time * trackaim;
					trackpos += trackaim * VIEW_AIM;
				}
			}

			// push camera transform
			glPushMatrix();

			// set camera to track position
			glTranslatef( -trackpos.x, -trackpos.y, 0 );

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

			// render all entities
			// (send interpolation ratio and offset from simulation time)
			Renderable::RenderAll(sim_timer, sim_step);

			// reset camera transform
			glPopMatrix();

			// if performing motion blur...
			if (RENDER_MOTIONBLUR > 1)
			{
				// accumulate the image
				glAccum(blur ? GL_ACCUM : GL_LOAD, 1.0f / float(RENDER_MOTIONBLUR));
			}
		}

		// if performing motion blur...
		if (RENDER_MOTIONBLUR > 1)
		{
			// return the accumulated image
			glAccum(GL_RETURN, 1);
		}

		// switch blend mode
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


		// for each player...
		for (Database::Typed<Player *>::Iterator itor(&Database::player); itor.IsValid(); ++itor)
		{
			// draw player health (HACK)
			Damagable *damagable = Database::damagable.Get(itor.GetKey());
			if (damagable)
			{
				// health ratio
				const DamagableTemplate &damagabletemplate = Database::damagabletemplate.Get(itor.GetKey());
				float health = damagable->GetHealth() / damagabletemplate.mHealth;

				// push camera transform
				glPushMatrix();

				// use 640x480 screen coordinates
				glLoadIdentity();
				glScalef( 1.0f / 640, 1.0f / 640, -1.0f );
				glTranslatef(-0.5f*640, -0.5f*640*SCREEN_HEIGHT/SCREEN_WIDTH, 1.0f);

				glBegin(GL_QUADS);

				// set color based on health
				if (health < 0.5f)
					glColor4f(1.0f, 0.1f + health * 0.9f / 0.5f, 0.1f, 1.0f - health * 0.9f);
				else if (health < 100)
					glColor4f(0.1f + (1.0f - health) * 0.9f / 0.5f, 1.0f, 0.1f, 1.0f - health * 0.9f);
				else
					glColor4f(0.1f, 1.0f, 0.1f, 0.1f);

				// fill gauge
				glVertex2f(8, 8);
				glVertex2f(8 + 100 * health, 8);
				glVertex2f(8 + 100 * health, 16);
				glVertex2f(8, 16);

				// background
				glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
				glVertex2f(8 + 100 * health, 8);
				glVertex2f(108, 8);
				glVertex2f(108, 16);
				glVertex2f(8 + 100 * health, 16);

				glEnd();

				// reset camera transform
				glPopMatrix();
			}
		}

		// restore blend mode
		glPopAttrib();

		/* Render our console */
		OGLCONSOLE_Draw();

		// show the screen
		SDL_GL_SwapBuffers();

#ifdef PRINT_PERFORMANCE_DETAILS
		LARGE_INTEGER perf_count1;
		QueryPerformanceCounter(&perf_count1);

		DebugPrint("R=%d\n", 1000000 * (perf_count1.QuadPart - perf_count0.QuadPart) / perf_freq.QuadPart);
#endif
	}
	while( !quit );

	DebugPrint("Quitting...\n");

	// clear all databases
	Database::Cleanup();

	// collidable done
	Collidable::WorldDone();

	// clean up
	clean_up();

	// done
	return 0;
}
