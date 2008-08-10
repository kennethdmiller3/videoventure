// precompiled header
#include "stdafx.h"

// includes
#include "oglconsole.h"                                                                              
#include "World.h"
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
#include "Sound.h"

#ifdef USE_VARIABLE
#include "Variable.h"
#endif

#ifdef USE_PATHING
#include "Pathing.h"
#endif

#include <malloc.h>

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
bool OPENGL_SWAPCONTROL = true;
bool OPENGL_ANTIALIAS = false;
int OPENGL_MULTISAMPLE = 4;

// debug output
bool DEBUGPRINT_OUTPUTCONSOLE = false;
bool DEBUGPRINT_OUTPUTDEBUG = false;
bool DEBUGPRINT_OUTPUTSTDERR = false;

// visual profiler
bool PROFILER_OUTPUTSCREEN = false;
bool PROFILER_OUTPUTPRINT = false;

// simulation attributes
int SIMULATION_RATE = 60;
float TIME_SCALE = 1.0f;
bool FIXED_STEP = false;

// rendering attributes
int MOTIONBLUR_STEPS = 1;
float MOTIONBLUR_TIME = 1.0f/60.0f;

// sound attributes
int SOUND_CHANNELS = 8;
float SOUND_VOLUME = 1.0f;

// default input configuration
std::string INPUT_CONFIG = "input.xml";

// default level configuration
std::string LEVEL_CONFIG = "level.xml";

// default record configuration
std::string RECORD_CONFIG = "record.xml";
bool record = false;
bool playback = false;

// runtime
bool runtime = false;

// device was reset
bool wasreset = true;

// console
OGLCONSOLE_Console console;

// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_CreateFont();
extern "C" void OGLCONSOLE_Resize(OGLCONSOLE_Console console);

#define GET_PERFORMANCE_DETAILS
#define PRINT_PERFORMANCE_DETAILS
#define DRAW_PERFORMANCE_DETAILS
//#define PRINT_SIMULATION_TIMER
#define TRACE_OPENGL_ATTRIBUTES

// input system
Input input;

// listener position (HACK)
Vector2 listenerpos;

// reticule handle (HACK)
GLuint reticule_handle;
GLuint score_handle;
GLuint lives_handle;

// forward declaration
int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );
void RunPlayState();

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
	if (DEBUGPRINT_OUTPUTSTDERR)
		fputs(buf, stderr);
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
	ProcessCommand(command, param, count);
}

bool init_GL()
{	
	// set clear color
	glClearColor( 0, 0, 0, 0 );

	if (OPENGL_ANTIALIAS)
	{
		// enable point smoothing
		glEnable( GL_POINT_SMOOTH );
		glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );

		// enable line smoothing
 		glEnable( GL_LINE_SMOOTH );
		glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

		// enable polygon smoothing
		glEnable( GL_POLYGON_SMOOTH );
		glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	}
	else
	{
		// disable point smoothing
		glDisable( GL_POINT_SMOOTH );

		// disable line smoothing
 		glDisable( GL_LINE_SMOOTH );

		// disable polygon smoothing
		glDisable( GL_POLYGON_SMOOTH );
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
	glFogf( GL_FOG_START, 256.0f*2.0f );
	glFogf( GL_FOG_END, 256.0f*5.0f );
#endif

	// return true if no errors
	return glGetError() == GL_NO_ERROR;
}

bool init_Window()
{
	// set OpenGL attributes
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
#ifdef ENABLE_SRC_ALPHA_SATURATE
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
#else
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
#endif
    SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 16 );
#ifdef ENABLE_SRC_ALPHA_SATURATE
    SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 16 );
#else
    SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0 );
#endif
#ifndef ENABLE_DEPTH_TEST
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
#endif
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, OPENGL_MULTISAMPLE > 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, OPENGL_MULTISAMPLE );
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, OPENGL_SWAPCONTROL );

	// create the window
	unsigned int flags = SDL_OPENGL;
	if (SCREEN_FULLSCREEN)
		flags |= SDL_FULLSCREEN;
	if( SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, flags ) == NULL )
		return false;

	// device was reset
	wasreset = true;

	// initialize OpenGL
	if( !init_GL() )
		return false;    

	if (runtime)
	{
		// rebuild draw lists
		RebuildDrawlists();

		// TO DO: rebuild textures
		OGLCONSOLE_CreateFont();
		OGLCONSOLE_Resize(console);
	}

	return true;
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

	// initialize the window
	init_Window();

	// hide the mouse cursor
	SDL_ShowCursor(SDL_DISABLE);

	// grab the cursor
	SDL_WM_GrabInput(SDL_GRAB_ON);

	// set window title
	SDL_WM_SetCaption( "Shmup!", NULL );

    /* Initialize OGLCONSOLE */                                                                      
    console = OGLCONSOLE_Create();                                                                             
    OGLCONSOLE_EnterKey(cmdCB);                                                                      

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

	// get fullscreen resolutions
	DebugPrint("\nResolutions:\n");
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
	for (SDL_Rect **mode = modes; *mode != NULL; ++mode)
		DebugPrint("%dx%d\n", (*mode)->w, (*mode)->h);
#endif

	SDL_AudioSpec fmt;
	fmt.freq = AUDIO_FREQUENCY;
	fmt.format = AUDIO_S16SYS;
	fmt.channels = 2;
	fmt.samples = Uint16(AUDIO_FREQUENCY / SIMULATION_RATE);
	fmt.callback = Sound::Mix;
	fmt.userdata = &listenerpos;

	/* Open the audio device and start playing sound! */
	if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
		DebugPrint("Unable to open audio: %s\n", SDL_GetError());
	}
	SDL_PauseAudio(0);

	// success!
	return true;    
}

void clean_up()
{
    /* clean up oglconsole */                                                                        
    OGLCONSOLE_Quit();

	SDL_CloseAudio();

	// quit SDL
	SDL_Quit();
}


void init_Input(const char *config)
{
	// clear existing bindings
	input.Clear();

	// load input binding file
	DebugPrint("Input %s\n", config);
	TiXmlDocument document(config);
	document.LoadFile();

	// process child elements of the root
	if (const TiXmlElement *root = document.FirstChildElement("input"))
		input.Configure(root);
}

void init_Level(const char *config)
{
	// stop any startup sound (HACK)
	StopSound(0x94326baa /* "startup" */);

	// clear existing level
	Database::Cleanup();

	// load level data file
	DebugPrint("Level %s\n", config);
	TiXmlDocument document(config);
	document.LoadFile();

	// process child elements of world
	if (const TiXmlElement *root = document.FirstChildElement("world"))
		ProcessWorldItems(root);

	// get the reticule draw list (HACK)
	reticule_handle = Database::drawlist.Get(0x170e4c58 /* "reticule" */);

	// play the startup sound (HACK)
	PlaySound(0x94326baa /* "startup" */);
}


// post-command function
typedef void (*ProcessCommandPostFunc)(void);

// process a string command
static int ProcessCommandString(std::string &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = aParam[0];
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue.c_str());
		return 0;
	}
}

// process a boolean command
static int ProcessCommandBool(bool &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = atoi(aParam[0]) != 0;
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue);
		return 0;
	}
}

// process an integer command
static int ProcessCommandInt(int &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = atoi(aParam[0]);
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue);
		return 0;
	}
}

// process a two-integer command
static int ProcessCommandInt2(int &aValue1, int &aValue2, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 2)
	{
		aValue1 = atoi(aParam[0]);
		aValue2 = atoi(aParam[1]);
		if (aAction)
			aAction();
		return 2;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue1, aValue2);
		return 0;
	}
}

// process a float command
static int ProcessCommandFloat(float &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = float(atof(aParam[0]));
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue);
		return 0;
	}
}

void InitWindowAction()
{
	if (runtime)
		init_Window();
}
void InitInputAction()
{
	if (runtime)
		init_Input(INPUT_CONFIG.c_str());
}
void InitLevelAction()
{
	if (runtime)
		init_Level(LEVEL_CONFIG.c_str());
}
void InitRecordAction()
{
	record = 1;
}
void InitPlaybackAction()
{
	playback = 1;
}
void ClampMotionBlurAction()
{
	if (MOTIONBLUR_STEPS < 1)
		MOTIONBLUR_STEPS = 1;
}

// commands
int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount )
{
	switch (aCommand)
	{
	case 0x1d215c8f /* "resolution" */:
		return ProcessCommandInt2(SCREEN_WIDTH, SCREEN_HEIGHT, aParam, aCount, InitWindowAction, "resolution: %dx%d\n"); 

	case 0xfe759eea /* "depth" */:
		return ProcessCommandInt(SCREEN_DEPTH, aParam, aCount, InitWindowAction, "depth: %dbpp");

	case 0x5032fb58 /* "fullscreen" */:
		return ProcessCommandBool(SCREEN_FULLSCREEN, aParam, aCount, InitWindowAction, "fullscreen: %d\n");

	case 0x06f8f066 /* "vsync" */:
		return ProcessCommandBool(OPENGL_SWAPCONTROL, aParam, aCount, InitWindowAction, "vsync: %d\n");

	case 0x35c8978f /* "antialias" */:
		return ProcessCommandBool(OPENGL_ANTIALIAS, aParam, aCount, InitWindowAction, "antialias: %d\n");

	case 0x47d0f228 /* "multisample" */:
		return ProcessCommandInt(OPENGL_MULTISAMPLE, aParam, aCount, InitWindowAction, "multisample: %d\n");

	case 0x1ae79789 /* "viewsize" */:
		return ProcessCommandFloat(VIEW_SIZE, aParam, aCount, NULL, "viewsize: %f\n");

	case 0x8e6b4341 /* "viewaim" */:
		return ProcessCommandFloat(VIEW_AIM, aParam, aCount, NULL, "viewaim: %f\n");

	case 0xd49cb7d3 /* "viewaimfilter" */:
		return ProcessCommandFloat(VIEW_AIM_FILTER, aParam, aCount, NULL, "viewaimfilter: %f\n");

	case 0xf9d86f7b /* "input" */:
		return ProcessCommandString(INPUT_CONFIG, aParam, aCount, InitInputAction, "input: %s\n");

	case 0x9b99e7dd /* "level" */:
		return ProcessCommandString(LEVEL_CONFIG, aParam, aCount, InitLevelAction, "level: %s\n");

	case 0x593058cc /* "record" */:
		return ProcessCommandString(RECORD_CONFIG, aParam, aCount, InitRecordAction, "record: %s\n");

	case 0xcf8a43ec /* "playback" */:
		return ProcessCommandString(RECORD_CONFIG, aParam, aCount, InitPlaybackAction, "playback: %s\n");

	case 0xd6974b06 /* "simrate" */:
		return ProcessCommandInt(SIMULATION_RATE, aParam, aCount, NULL, "simrate: %d\n");

	case 0x9f2f269e /* "timescale" */:
		return ProcessCommandFloat(TIME_SCALE, aParam, aCount, NULL, "timescale: %f\n");

	case 0xe065cb63 /* "fixedstep" */:
		return ProcessCommandBool(FIXED_STEP, aParam, aCount, NULL, "fixedstep: %d\n");

	case 0xf744f3b2 /* "motionblur" */:
		return ProcessCommandInt(MOTIONBLUR_STEPS, aParam, aCount, ClampMotionBlurAction, "motionblur: %d\n");

	case 0xfb585f73 /* "motionblurtime" */:
		return ProcessCommandFloat(MOTIONBLUR_TIME, aParam, aCount, NULL, "motionblurtime: %f\n");

	case 0x61e734dc /* "soundchannels" */:
		return ProcessCommandInt(SOUND_CHANNELS, aParam, aCount, NULL, "soundchannels: %d\n");

	case 0x2ac3f7e6 /* "soundvolume" */:
		return ProcessCommandFloat(SOUND_VOLUME, aParam, aCount, NULL, "soundvolume: %f\n");
		
	case 0x94c716fd /* "outputconsole" */:
		return ProcessCommandBool(DEBUGPRINT_OUTPUTCONSOLE, aParam, aCount, NULL, "outputconsole: %d\n");

	case 0x54822903 /* "outputdebug" */:
		return ProcessCommandBool(DEBUGPRINT_OUTPUTDEBUG, aParam, aCount, NULL, "outputdebug: %d\n");

	case 0x8940763c /* "outputstderr" */:
		return ProcessCommandBool(DEBUGPRINT_OUTPUTSTDERR, aParam, aCount, NULL, "outputstderr: %d\n");

	case 0xfbcc8f02 /* "profilescreen" */:
		return ProcessCommandBool(PROFILER_OUTPUTSCREEN, aParam, aCount, NULL, "profilescreen: %d\n");

	case 0x85e872f9 /* "profileprint" */:
		return ProcessCommandBool(PROFILER_OUTPUTPRINT, aParam, aCount, NULL, "profileprint: %d\n");

	case 0xa165ddb8 /* "database" */:
		if (aCount >= 1)
		{
			// get the database identifier
			unsigned int id;
			if (!sscanf(aParam[0], "0x%x", &id))
				id = Hash(aParam[0]);

			// get the dtabase
			Database::Untyped *db = Database::GetDatabases().Get(id);
			if (db)
			{
				// list database properties
				OGLCONSOLE_Output(console, "stride=%d shift=%d bits=%d limit=%d count=%d\n",
					db->GetStride(), db->GetShift(), db->GetBits(), db->GetLimit(), db->GetCount());
			}
			else
			{
				// not found
				OGLCONSOLE_Output(console, "database \"%s\" (0x%08x) not found\n", aParam[0], id);
			}
			return 1;
		}
		else
		{
			// list all database identifiers
			OGLCONSOLE_Output(console, "databases:\n");
			for (Database::Untyped::Iterator itor(&Database::GetDatabases()); itor.IsValid(); ++itor)
			{
				OGLCONSOLE_Output(console, "0x%08x\n", itor.GetKey());
			}
			return 0;
		}

	case 0xbdf0855a /* "find" */:
		if (aCount >= 1)
		{
			// get the database identifier
			unsigned int id;
			if (!sscanf(aParam[0], "0x%x", &id))
				id = Hash(aParam[0]);

			// get the database
			Database::Untyped *db = Database::GetDatabases().Get(id);
			if (db)
			{
				// if supplying a key...
				if (aCount >= 2)
				{
					// if the key is an identifier...
					unsigned int key = 0;
					if (sscanf(aParam[1], "0x%x", &key))
					{
						// look up the identifier
						if (const void *value = db->Find(key))
						{
							const std::string &name = Database::name.Get(key);
							OGLCONSOLE_Output(console, "name=\"%s\" key=0x%08x data=0x%p\n", name.c_str(), key, value);
						}
						else
						{
							OGLCONSOLE_Output(console, "no record found\n");
						}
					}
					else
					{
						// list all keys matching the string name
						OGLCONSOLE_Output(console, "records matching \"%s\":\n", aParam[1]);
						for (Database::Untyped::Iterator itor(db); itor.IsValid(); ++itor)
						{
							const std::string &name = Database::name.Get(itor.GetKey());
							if (_stricmp(name.c_str(), aParam[1]) == 0)
							{
								OGLCONSOLE_Output(console, "%d: name=\"%s\" key=0x%08x data=0x%p\n", itor.GetSlot(), name.c_str(), itor.GetKey(), itor.GetValue());
							}
						}
					}
					return 2;
				}
				else
				{
					// list the contents of the database
					for (Database::Untyped::Iterator itor(db); itor.IsValid(); ++itor)
					{
						const std::string &name = Database::name.Get(itor.GetKey());
						OGLCONSOLE_Output(console, "%d: name=\"%s\" key=0x%08x data=0x%p\n", itor.GetSlot(), name.c_str(), itor.GetKey(), itor.GetValue());
					}
				}
			}
			else
			{
				// not found
				OGLCONSOLE_Output(console, "database \"%s\" (0x%08x) not found\n", aParam[0], id);
			}
			return 1;
		}
		else
		{
			return 0;
		}

	case 0x1bf51169 /* "components" */:
		if (aCount >= 1)
		{
			// get the identifier
			unsigned int key;
			if (!sscanf(aParam[0], "0x%x", &key))
				key = Hash(aParam[0]);

			// for each database...
			for (Database::Typed<Database::Untyped *>::Iterator itor(&Database::GetDatabases()); itor.IsValid(); ++itor)
			{
				// if the database contains a record for the identifier...
				if (const void *value = itor.GetValue()->Find(key))
				{
					OGLCONSOLE_Output(console, "database 0x%08x data=0x%p\n", itor.GetKey(), value);
				}
			}
			return 1;
		}
		else
		{
			return 0;
		}

	case 0x0e0d9594 /* "sound" */:
		if (aCount >= 2)
		{
			PlaySound(Hash(aParam[0]), Hash(aParam[1]));
			return 2;
		}
		else
		{
			return 0;
		}
		
	default:
#ifdef USE_VARIABLE
		// check variable system
		if (VarItem *item = Database::varitem.Get(aCommand))
		{
			if (item->mType == VarItem::COMMAND)
			{
				// trigger the command
				item->Notify();
				return 0;
			}
			else
			{
				if (aCount >= 1)
				{
					// set value
					item->SetString(aParam[0]);
					return 1;
				}
				else
				{
					// get value
					OGLCONSOLE_Output(console, "%s\n", item->GetString().c_str());
					return 0;
				}
			}
		}
		else
		{
			return 0;
		}
#else
		return 0;
#endif
	}
}

#ifdef _MSC_VER
// Is a debugger attached?
// Created by Tim Chew
// http://www.codeproject.com/KB/debug/debugger.aspx
BOOL IsDebuggerAttached()
{
    DWORD dw;

    __asm
    {
        push eax    // Preserve the registers
        push ecx
        mov eax, fs:[0x18]  // Get the TIB's linear address
        mov eax, dword ptr [eax + 0x30]
        mov ecx, dword ptr [eax]    // Get the whole DWORD
        mov dw, ecx // Save it
        pop ecx // Restore the registers
        pop eax
    }

    // The 3rd byte is the byte we really need to check for the
    // presence of a debugger.
    // Check the 3rd byte
    return (BOOL)(dw & 0x00010000 ? TRUE : FALSE);
}
#endif

// main
int SDL_main( int argc, char *argv[] )
{
#ifdef _MSC_VER
	if (IsDebuggerAttached())
	{
		// turn on floating-point exceptions
		unsigned int prev;
		_controlfp_s(&prev, unsigned int(~(_EM_ZERODIVIDE|_EM_INVALID)), _MCW_EM);

		// enable debug heap in a debug build
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_EVERY_1024_DF|_CRTDBG_CHECK_CRT_DF|_CRTDBG_LEAK_CHECK_DF );
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	}
#endif

	// process command-line arguments
	for (int i = 1; i < argc; ++i)
	{
		// if the argument is a command...
		if (argv[i][0] == '-' || argv[i][0] == '/')
		{
			// get command hash
			unsigned int command = Hash(argv[i]+1);

			// scan for next command
			int count = 0;
			for (int j = i+1; j < argc; j++)
			{
				if (argv[j][0] == '-' || argv[j][0] == '/')
				{
					break;
				}
				++count;
			}

			ProcessCommand(command, argv+i+1, count);
			i += count;
		}
	}

	// initialize
	if( !init() )
		return 1;    
	
	RunPlayState();

	// clean up
	clean_up();

	// done
	return 0;
}

void RunPlayState()
{
	// clear the screen
	glClear(
		GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_BUFFER
		| GL_DEPTH_BUFFER_BIT
#endif
		);

	// show the screen
	SDL_GL_SwapBuffers();

	// collidable initialization
	Collidable::WorldInit();

	// input binding
	init_Input(INPUT_CONFIG.c_str());

	// level configuration
	init_Level(LEVEL_CONFIG.c_str());


	//

	// allocate score draw list
	score_handle = glGenLists(1);

	// allocate lives draw list
	lives_handle = glGenLists(1);

	// last ticks
	unsigned int ticks = SDL_GetTicks();

	// simulation timer
	const float sim_rate = float(SIMULATION_RATE);
	const float sim_step = 1.0f / sim_rate;
	float sim_turns = 1.0f;
	unsigned int sim_turn = 0;

	// pause state
	bool paused = false;
	bool singlestep = false;

	// input logging
	TiXmlDocument inputlog(RECORD_CONFIG.c_str());
	TiXmlElement *inputlogroot;
	TiXmlElement *inputlognext;
	if (playback)
	{
		inputlog.LoadFile();
		inputlogroot = inputlog.RootElement();
		inputlognext = inputlogroot->FirstChildElement();
	}
	else if (record)
	{
		inputlogroot = inputlog.LinkEndChild(new TiXmlElement("journal"))->ToElement();
		inputlognext = NULL;
	}
	else
	{
		inputlogroot = NULL;
		inputlognext = NULL;
	}

	// camera track position
	Vector2 trackpos[2] = { Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f) };
	Vector2 trackaim(0.0f, 0.0f);

	// aim track position
	Vector2 aimpos[2] = { Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f)  };

#ifdef GET_PERFORMANCE_DETAILS
	LARGE_INTEGER perf_freq;
	QueryPerformanceFrequency(&perf_freq);

	static const int NUM_SAMPLES = 640;
	LONGLONG control_time[NUM_SAMPLES] = { 0 };
	LONGLONG simulate_time[NUM_SAMPLES] = { 0 };
	LONGLONG collide_time[NUM_SAMPLES] = { 0 };
	LONGLONG update_time[NUM_SAMPLES] = { 0 };
	LONGLONG render_time[NUM_SAMPLES] = { 0 };
	LONGLONG display_time[NUM_SAMPLES] = { 0 };
	int profile_index = -1;
#endif

#ifdef COLLECT_DEBUG_DRAW
	// create a new draw list
	GLuint debugdraw = glGenLists(1);
#endif

	DebugPrint("Simulating at %dHz (x%f)\n", SIMULATION_RATE, TIME_SCALE);

	// set to runtime mode
	runtime = true;

	// quit flag
	bool quit = false;

	// wait for user exit
	do
	{

#ifdef GET_PERFORMANCE_DETAILS
//		if (!paused)
		if (++profile_index >= NUM_SAMPLES)
			profile_index = 0;
		control_time[profile_index] = 0;
		simulate_time[profile_index] = 0;
		collide_time[profile_index] = 0;
		update_time[profile_index] = 0;
		render_time[profile_index] = 0;
		display_time[profile_index] = 0;
#endif

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
				input.OnPress( Input::TYPE_KEYBOARD, event.key.which, event.key.keysym.sym );
				if ((event.key.keysym.sym == SDLK_F4) && (event.key.keysym.mod & KMOD_ALT))
				{
					quit = true;
				}
				if ((event.key.keysym.sym == SDLK_RETURN) && (event.key.keysym.mod & KMOD_ALT))
				{
					SCREEN_FULLSCREEN = !SCREEN_FULLSCREEN;
					init_Window();
				}
				else if (event.key.keysym.sym == SDLK_PAUSE)
				{
					if (event.key.keysym.mod & KMOD_SHIFT)
					{
						paused = true;
						singlestep = true;
					}
					else
					{
						paused = !paused;
					}
					SDL_ShowCursor(paused ? SDL_ENABLE : SDL_DISABLE);
					SDL_WM_GrabInput(paused ? SDL_GRAB_OFF : SDL_GRAB_ON);
					SDL_PauseAudio(paused);
				}
				break;
			case SDL_KEYUP:
				input.OnRelease( Input::TYPE_KEYBOARD, event.key.which, event.key.keysym.sym );
				break;
			case SDL_MOUSEMOTION:
				input.OnAxis( Input::TYPE_MOUSE_AXIS, event.motion.which, 0, float(event.motion.x * 2 - SCREEN_WIDTH) / float(SCREEN_HEIGHT) );
				input.OnAxis( Input::TYPE_MOUSE_AXIS, event.motion.which, 1, float(event.motion.y * 2 - SCREEN_HEIGHT) / float(SCREEN_HEIGHT) );
				input.OnAxis( Input::TYPE_MOUSE_AXIS, event.motion.which, 2, event.motion.xrel / 32.0f );
				input.OnAxis( Input::TYPE_MOUSE_AXIS, event.motion.which, 3, event.motion.yrel / 32.0f );
				break;
			case SDL_MOUSEBUTTONDOWN:
				input.OnPress( Input::TYPE_MOUSE_BUTTON, event.button.which, event.button.button );
				break;
			case SDL_MOUSEBUTTONUP:
				input.OnRelease( Input::TYPE_MOUSE_BUTTON, event.button.which, event.button.button );
				break;
			case SDL_JOYAXISMOTION:
				input.OnAxis( Input::TYPE_JOYSTICK_AXIS, event.jaxis.which, event.jaxis.axis, event.jaxis.value / 32767.0f );
				break;
			case SDL_JOYBUTTONDOWN:
				input.OnPress( Input::TYPE_JOYSTICK_BUTTON, event.jaxis.which, event.jbutton.button );
				break;
			case SDL_JOYBUTTONUP:
				input.OnRelease( Input::TYPE_JOYSTICK_BUTTON, event.jbutton.which, event.jbutton.button );
				break;
			case SDL_QUIT:
				quit = true;
				break;
			}
		}

		// get loop time in ticks
		unsigned int delta = SDL_GetTicks() - ticks;
		ticks += delta;

		// clamp ticks to something sensible
		// (while debugging, for example)
		if (delta > 100)
			delta = 100;

		// delta time
		float delta_time, delta_turns;

		if (singlestep)
		{
			singlestep = false;

			// advance one frame
			delta_time = TIME_SCALE / 60.0f / MOTIONBLUR_STEPS;

			// turns to advance per step
			delta_turns = delta_time * sim_rate;
		}
		else if (paused)
		{
			// freeze time
			delta_time = 0.0f;

			// turns to advance per step
			delta_turns = TIME_SCALE / 60.0f / MOTIONBLUR_STEPS * sim_rate;

			// set turn counter to almost reach a new turn
			sim_turns = 0.99609375f - delta_turns * MOTIONBLUR_STEPS;
		}
		else if (FIXED_STEP)
		{
			// turns to advance per step
			delta_turns = TIME_SCALE / MOTIONBLUR_STEPS;

			// time to advance per step
			delta_time = delta_turns * sim_step;

		}
		else
		{
			// time to advance per step
			delta_time = delta * TIME_SCALE / 1000.0f / MOTIONBLUR_STEPS;

			// turns to advance per step
			delta_turns = delta_time * sim_rate;
		}

		// for each motion-blur step
		for (int blur = 0; blur < MOTIONBLUR_STEPS; ++blur)
		{
			// clear the screen
			glClear(
				GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_BUFFER
				| GL_DEPTH_BUFFER_BIT
#endif
				);

			// set projection
			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
			glFrustum( -0.5*VIEW_SIZE, 0.5*VIEW_SIZE, 0.5f*VIEW_SIZE*SCREEN_HEIGHT/SCREEN_WIDTH, -0.5f*VIEW_SIZE*SCREEN_HEIGHT/SCREEN_WIDTH, 256.0f*1.0f, 256.0f*5.0f );

			// set base modelview matrix
			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();
			glTranslatef( 0.0f, 0.0f, -256.0f );
			glScalef( -1.0f, -1.0f, -1.0f );

			// advance the sim timer
			sim_turns += delta_turns;

			// while simulation turns to run...
			while (sim_turns >= 1.0f)
			{
#ifdef COLLECT_DEBUG_DRAW
				// collect any debug draw
				glNewList(debugdraw, GL_COMPILE);
#endif

				// deduct a turn
				sim_turns -= 1.0f;
				
				// update database
				Database::Update();

				if (playback)
				{
					// quit if out of turns
					if (!inputlognext)
					{
						quit = true;
						break;
					}

					// get the next turn value
					int turn = -1;
					inputlognext->QueryIntAttribute("turn", &turn);

					// if the turn matches the simulation turn...
					if ((unsigned int)turn == sim_turn)
					{
						// update the control values
						inputlognext->QueryIntAttribute("move_x", reinterpret_cast<int *>(&input.output[Input::MOVE_HORIZONTAL]));
						inputlognext->QueryIntAttribute("move_y", reinterpret_cast<int *>(&input.output[Input::MOVE_VERTICAL]));
						inputlognext->QueryIntAttribute("aim_x", reinterpret_cast<int *>(&input.output[Input::AIM_HORIZONTAL]));
						inputlognext->QueryIntAttribute("aim_y", reinterpret_cast<int *>(&input.output[Input::AIM_VERTICAL]));
						inputlognext->QueryIntAttribute("fire1", reinterpret_cast<int *>(&input.output[Input::FIRE_PRIMARY]));
						inputlognext->QueryIntAttribute("fire2", reinterpret_cast<int *>(&input.output[Input::FIRE_SECONDARY]));

						// go to the next entry
						inputlognext = inputlognext->NextSiblingElement();
					}
				}
				else if (record)
				{
					// save original input values
					float prev[Input::NUM_LOGICAL];
					memcpy(prev, input.output, sizeof(prev));

					// update input values
					input.Update();

					// if any controls have changed...
					if (memcmp(prev, input.output, sizeof(prev)) != 0)
					{
						// create an input turn entry
						TiXmlElement item( "input" );
						item.SetAttribute( "turn", sim_turn );

						// add changed control values
						if (input.output[Input::MOVE_HORIZONTAL] != prev[Input::MOVE_HORIZONTAL])
							item.SetAttribute( "move_x", *reinterpret_cast<int *>(&input.output[Input::MOVE_HORIZONTAL]));
						if (input.output[Input::MOVE_VERTICAL] != prev[Input::MOVE_VERTICAL])
							item.SetAttribute( "move_y", *reinterpret_cast<int *>(&input.output[Input::MOVE_VERTICAL]));
						if (input.output[Input::AIM_HORIZONTAL] != prev[Input::AIM_HORIZONTAL])
							item.SetAttribute( "aim_x", *reinterpret_cast<int *>(&input.output[Input::AIM_HORIZONTAL]));
						if (input.output[Input::AIM_VERTICAL] != prev[Input::AIM_VERTICAL])
							item.SetAttribute( "aim_y", *reinterpret_cast<int *>(&input.output[Input::AIM_VERTICAL]));
						if (input.output[Input::FIRE_PRIMARY] != prev[Input::FIRE_PRIMARY])
							item.SetAttribute( "fire1", *reinterpret_cast<int *>(&input.output[Input::FIRE_PRIMARY]));
						if (input.output[Input::FIRE_SECONDARY] != prev[Input::FIRE_SECONDARY])
							item.SetAttribute( "fire2", *reinterpret_cast<int *>(&input.output[Input::FIRE_SECONDARY]));

						// add the new input entry
						inputlogroot->InsertEndChild(item);
					}
				}
				else
				{
					// update input values
					input.Update();
				}


				// CONTROL PHASE

#ifdef GET_PERFORMANCE_DETAILS
				LARGE_INTEGER perf_count0;
				QueryPerformanceCounter(&perf_count0);
#endif

				// control all entities
				Controller::ControlAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				LARGE_INTEGER perf_count1;
				QueryPerformanceCounter(&perf_count1);
				control_time[profile_index] += perf_count1.QuadPart - perf_count0.QuadPart;
#endif

				// SIMULATION PHASE
				// (generate forces)
				Simulatable::SimulateAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				LARGE_INTEGER perf_count2;
				QueryPerformanceCounter(&perf_count2);
				simulate_time[profile_index] += perf_count2.QuadPart - perf_count1.QuadPart;
#endif

				// COLLISION PHASE
				// (apply forces and update positions)
				Collidable::CollideAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				LARGE_INTEGER perf_count3;
				QueryPerformanceCounter(&perf_count3);
				collide_time[profile_index] += perf_count3.QuadPart - perf_count2.QuadPart;
#endif

				// UPDATE PHASE
				// (use updated positions)
				Updatable::UpdateAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				LARGE_INTEGER perf_count4;
				QueryPerformanceCounter(&perf_count4);
				update_time[profile_index] += perf_count4.QuadPart - perf_count3.QuadPart;
#endif

				// for each player...
				for (Database::Typed<PlayerController *>::Iterator itor(&Database::playercontroller); itor.IsValid(); ++itor)
				{
					// TO DO: support multiple players

					// get the entity
					Entity *entity = Database::entity.Get(itor.GetKey());

					// track player position
					trackpos[0] = trackpos[1];
					trackpos[1] = entity->GetPosition();

					// update target aim position
					aimpos[0] = aimpos[1];
					aimpos[1] = Vector2(input[Input::AIM_HORIZONTAL], input[Input::AIM_VERTICAL]);

					// set listener position
					listenerpos = trackpos[1];

					// if applying view aim
					if (VIEW_AIM)
					{
						Vector2 trackdelta;
						if (Database::ship.Get(itor.GetKey()))
							trackdelta = aimpos[1] - trackaim;
						else
							trackdelta = -trackaim;
						if (trackdelta.LengthSq() > FLT_EPSILON)
							trackaim += VIEW_AIM_FILTER * sim_step * trackdelta;
						trackpos[1] += trackaim * VIEW_AIM;
					}

#ifdef TEST_PATHING
					Pathing(entity->GetPosition(), trackpos[1] + aimpos[1] * 240 * VIEW_SIZE / 640, 4.0f);
#endif
				}

				// step inputs for next turn
				input.Step();

				// advance the turn counter
				++sim_turn;
				Renderable::SetTurn(sim_turn);

#ifdef COLLECT_DEBUG_DRAW
				// finish the draw list
				glEndList();
#endif
			}

#ifdef PRINT_SIMULATION_TIMER
			DebugPrint("delta=%d ticks=%d sim_t=%f\n", delta, ticks, sim_turns);
#endif

#ifdef GET_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_count0;
			QueryPerformanceCounter(&perf_count0);
#endif

			// RENDERING PHASE

			// push camera transform
			glPushMatrix();

			// get interpolated track position
			Vector2 viewpos(Lerp(trackpos[0].x, trackpos[1].x, sim_turns), Lerp(trackpos[0].y, trackpos[1].y, sim_turns));

			// set view position
			glTranslatef( -viewpos.x, -viewpos.y, 0 );

			// calculate view area
			AlignedBox2 view;
			view.min.x = viewpos.x - VIEW_SIZE * 0.5f;
			view.max.x = viewpos.x + VIEW_SIZE * 0.5f;
			view.min.y = viewpos.y - VIEW_SIZE * 0.5f * SCREEN_HEIGHT / SCREEN_WIDTH;
			view.max.y = viewpos.y + VIEW_SIZE * 0.5f * SCREEN_HEIGHT / SCREEN_WIDTH;

			// render all entities
			// (send interpolation ratio and offset from simulation time)
			Renderable::RenderAll(sim_turns, sim_step, view);

			// reset camera transform
			glPopMatrix();

			// if performing motion blur...
			if (MOTIONBLUR_STEPS > 1)
			{
				// accumulate the image
				glAccum(blur ? GL_ACCUM : GL_LOAD, 1.0f / float(MOTIONBLUR_STEPS));
			}

#ifdef GET_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_count1;
			QueryPerformanceCounter(&perf_count1);
			render_time[profile_index] += perf_count1.QuadPart - perf_count0.QuadPart;
#endif
		}

#ifdef GET_PERFORMANCE_DETAILS
		LARGE_INTEGER perf_count0;
		QueryPerformanceCounter(&perf_count0);
#endif

		// if performing motion blur...
		if (MOTIONBLUR_STEPS > 1)
		{
			// return the accumulated image
			glAccum(GL_RETURN, 1);

			// return time to real time
			delta_time *= MOTIONBLUR_STEPS;
		}

		// switch blend mode
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

#ifdef COLLECT_DEBUG_DRAW
		// push camera transform
		glPushMatrix();

		// get interpolated track position
		Vector2 viewpos(Lerp(trackpos[0].x, trackpos[1].x, sim_turns), Lerp(trackpos[0].y, trackpos[1].y, sim_turns));

		// set camera to track position
		glTranslatef( -viewpos.x, -viewpos.y, 0 );

		// debug draw
		glCallList(debugdraw);

		// pop camera transform
		glPopMatrix();
#endif

		// push projection transform
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 640, 480, 0, -1, 1);

		// use 640x480 screen coordinates
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		// for each player...
		int playerindex = 0;
		for (Database::Typed<Player *>::Iterator itor(&Database::player); itor.IsValid(); ++itor)
		{
			// get the attached entity identifier
			unsigned int id = itor.GetValue()->mAttach;

			// draw player health (HACK)
			float health = 0.0f;
			Damagable *damagable = Database::damagable.Get(id);
			if (damagable)
			{
				// get health ratio
				const DamagableTemplate &damagabletemplate = Database::damagabletemplate.Get(id);
				health = damagable->GetHealth() / damagabletemplate.mHealth;
			}

			// fill gauge values
			static const int MAX_PLAYERS = 8;
			static float fill[MAX_PLAYERS] = { 0 };

			// drain values
			static const float DRAIN_DELAY = 1.0f;
			static const float DRAIN_RATE = 0.5f;
			static float drain[MAX_PLAYERS] = { 0 };
			static float draindelay[MAX_PLAYERS] = { 0 };

			// flash values
			static const int MAX_FLASH = 16;
			static const float FLASH_RATE = 2.0f;
			struct Flash
			{
				float left;
				float right;
				float fade;
			};
			static Flash flash[MAX_PLAYERS][MAX_FLASH] = { 0 };
			static int flashcount[MAX_PLAYERS] = { 0 };

			// if health is greater than the gauge fill...
			if (fill[playerindex] < health - FLT_EPSILON)
			{
				// raise the fill
				fill[playerindex] = health;
				if (drain[playerindex] < fill[playerindex])
					drain[playerindex] = fill[playerindex];
			}
			// else if health is lower than the gauge fill...
			else if (fill[playerindex] > health + FLT_EPSILON)
			{
				// add a flash
				if (flashcount[playerindex] == MAX_FLASH)
					--flashcount[playerindex];
				for (int i = flashcount[playerindex]; i > 0; --i)
					flash[playerindex][i] = flash[playerindex][i-1];
				Flash &flashinfo = flash[playerindex][0];
				flashinfo.left = health;
				flashinfo.right = fill[playerindex];
				flashinfo.fade = 1.0f;
				++flashcount[playerindex];

				// lower the fill
				fill[playerindex] = health;

				// reset the drain delay
				draindelay[playerindex] = DRAIN_DELAY;
			}

			// update pulse
			static float pulsetimer = 0.0f;
			pulsetimer += delta_time * (1.0f + (1.0f - health) * (1.0f - health) * 4.0f);
			while (pulsetimer >= 1.0f)
				pulsetimer -= 1.0f;
			float pulse = sinf(pulsetimer * float(M_PI));
			pulse *= pulse;
			pulse *= pulse;
			pulse *= pulse;

			// set color based on health and pulse
			static const float healthcolor[3][4] =
			{
				{ 1.0f, 0.0f, 0.0f, 1.0f },
				{ 1.0f, 1.0f, 0.0f, 0.75f },
				{ 0.0f, 1.0f, 0.0f, 0.5f }
			};
			static const float pulsecolor[3][4] =
			{
				{ 1.0f, 1.0f, 1.0f, 1.0f },
				{ 1.0f, 1.0f, 0.3f, 0.75f },
				{ 0.2f, 1.0f, 0.2f, 0.5f },
			};

			int band = (health > 0.5f);
			float ratio = health * 2.0f - band;

			float fillcolor[4];
			for (int i = 0; i < 4; i++)
				fillcolor[i] = Lerp(Lerp(healthcolor[band][i], healthcolor[band+1][i], ratio), Lerp(pulsecolor[band][i], pulsecolor[band+1][i], ratio), pulse);

			// begin drawing
			glBegin(GL_QUADS);

			// background
			glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
			glVertex2f(8, 8);
			glVertex2f(108, 8);
			glVertex2f(108, 16);
			glVertex2f(8, 16);

			// drain
			glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f(8 + 100 * fill[playerindex], 8);
			glVertex2f(8 + 100 * drain[playerindex], 8);
			glVertex2f(8 + 100 * drain[playerindex], 16);
			glVertex2f(8 + 100 * fill[playerindex], 16);

			// flash
			for (int i = 0; i < flashcount[playerindex]; ++i)
			{
				Flash &flashinfo = flash[playerindex][i];
				glColor4f(1.0f, 1.0f, 1.0f, flashinfo.fade);
				glVertex2f(8 + 100 * flashinfo.left, 8 - 2 * flashinfo.fade);
				glVertex2f(8 + 100 * flashinfo.right, 8 - 2 * flashinfo.fade);
				glVertex2f(8 + 100 * flashinfo.right, 16 + 2 * flashinfo.fade);
				glVertex2f(8 + 100 * flashinfo.left, 16 + 2 * flashinfo.fade);
			}

			// fill gauge
			glColor4fv(fillcolor);
			glVertex2f(8, 8);
			glVertex2f(8 + 100 * fill[playerindex], 8);
			glVertex2f(8 + 100 * fill[playerindex], 16);
			glVertex2f(8, 16);

			glEnd();

			// if the drain delay elapsed...
			draindelay[playerindex] -= delta_time;
			if (draindelay[playerindex] <= 0)
			{
				// update drain
				drain[playerindex] -= DRAIN_RATE * delta_time;
				if (drain[playerindex] < fill[playerindex])
					drain[playerindex] = fill[playerindex];
			}

			// count down flash timers
			for (int i = 0; i < flashcount[playerindex]; ++i)
			{
				Flash &flashinfo = flash[playerindex][i];
				flashinfo.fade -= FLASH_RATE * delta_time;
				if (flashinfo.fade <= 0.0f)
				{
					flashcount[playerindex] = i;
					break;
				}
			}

			// get player score
			static int cur_score = -1;
			int new_score = itor.GetValue()->mScore;

			// if the score has not changed...
			if (new_score == cur_score && !wasreset)
			{
				// call the existing draw list
				glCallList(score_handle);
			}
			else
			{
				// update score
				cur_score = new_score;

				// start a new draw list list
				glNewList(score_handle, GL_COMPILE_AND_EXECUTE);

				// draw player score (HACK)
				char score[9];
				sprintf(score, "%08d", new_score);
				bool leading = true;

				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);
				glBegin(GL_QUADS);

				float x = 8;
				float y = 32;
				float z = 0;
				float w = 16;
				float h = -16;
				static const float textcolor[2][3] =
				{
					{ 0.4f, 0.5f, 1.0f },
					{ 0.3f, 0.3f, 0.3f }
				};

				for (char *s = score; *s != '\0'; ++s)
				{
					char c = *s;
					if (c != '0')
						leading = false;
					glColor3fv(textcolor[leading]);
					OGLCONSOLE_DrawCharacter(c, x, y, w, h, z);
					x += w;
				}

				glEnd();

				glDisable(GL_TEXTURE_2D);

				glEndList();
			}

			int cur_lives = -1;
			int new_lives = itor.GetValue()->mLives;
			if (new_lives < INT_MAX)
			{
				// if the lives has not changed...
				if (new_lives == cur_lives && !wasreset)
				{
					// call the existing draw list
					glCallList(lives_handle);
				}
				else
				{
					// update lives
					cur_lives = new_lives;

					// start a new draw list list
					glNewList(lives_handle, GL_COMPILE_AND_EXECUTE);

					// draw remaining lives
					char lives[16];
					sprintf(lives, "x%d", cur_lives);

					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

					glColor4f(0.4f, 0.5f, 1.0f, 1.0f);

					glBegin(GL_QUADS);

					float x = 116;
					float y = 16;
					float z = 0;
					float w = 8;
					float h = -8;
					OGLCONSOLE_DrawString(lives, x, y, w, h, z);

					glEnd();

					glDisable(GL_TEXTURE_2D);

					glEndList();
				}
			}

			// draw reticule
			Controller *controller = Database::controller.Get(id);
			if (controller)
			{
				float x = 320 - 240 * Lerp(aimpos[0].x, aimpos[1].x, sim_turns);
				float y = 240 - 240 * Lerp(aimpos[0].y, aimpos[1].y, sim_turns);

				glPushMatrix();
				glTranslatef(x, y, 0.0f);
				glCallList(reticule_handle);
				glPopMatrix();
			}
			else if (cur_lives <= 0)
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);


				glBegin(GL_QUADS);

				float x = 320 - 32 * 4 - 16;
				float y = 240 - 16;
				float z = 0;
				float w = 32;
				float h = -32;

				glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
				OGLCONSOLE_DrawString("GAME OVER", x - 4, y, w, h, z);
				OGLCONSOLE_DrawString("GAME OVER", x + 4, y, w, h, z);
				OGLCONSOLE_DrawString("GAME OVER", x, y - 4, w, h, z);
				OGLCONSOLE_DrawString("GAME OVER", x, y + 4, w, h, z);

				glColor4f(1.0f, 0.9f, 0.1f, 1.0f);
				OGLCONSOLE_DrawString("GAME OVER", x, y, w, h, z);

				glEnd();

				glDisable(GL_TEXTURE_2D);
			}

			++playerindex;
		}

		// reset camera transform
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		// restore blend mode
		glPopAttrib();

#ifdef GET_PERFORMANCE_DETAILS
		if (!OPENGL_SWAPCONTROL)
		{
			LARGE_INTEGER perf_count1;
			QueryPerformanceCounter(&perf_count1);
			render_time[profile_index] += perf_count1.QuadPart - perf_count0.QuadPart;

			// wait for rendering to finish
			glFinish();

			LARGE_INTEGER perf_count2;
			QueryPerformanceCounter(&perf_count2);
			display_time[profile_index] += perf_count2.QuadPart - perf_count1.QuadPart;
		}
		else
		{
			// don't count display time
			display_time[profile_index] = 0;
		}

#ifdef DRAW_PERFORMANCE_DETAILS
		if (PROFILER_OUTPUTSCREEN)
		{
			// switch blend mode
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			// push projection transform
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0, 640, 480, 0, -1, 1);

			// use 640x480 screen coordinates
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			struct BandInfo
			{
				const LONGLONG * time;
				float r;
				float g;
				float b;
				float a;
			};
			static BandInfo band_info[6] =
			{
				{ control_time,		1.0f,	0.0f,	0.0f,	0.5f },
				{ simulate_time,	1.0f,	1.0f,	0.0f,	0.5f },
				{ collide_time,		0.0f,	1.0f,	0.0f,	0.5f },
				{ update_time,		0.0f,	0.5f,	1.0f,	0.5f },
				{ render_time,		1.0f,	0.0f,	1.0f,	0.5f },
				{ display_time,		0.5f,	0.5f,	0.5f,	0.5f },
			};

			// generate y samples
			float sample_y[7][NUM_SAMPLES];
			int index = profile_index;
			for (int i = 0; i < NUM_SAMPLES; ++i)
			{
				float y = 480.0f;
				sample_y[0][i] = y;
				for (int band = 0; band < 6; ++band)
				{
					y -= 60.0f * 480.0f * band_info[band].time[index] / perf_freq.QuadPart;
					sample_y[band+1][i] = y;
				}
				if (++index >= NUM_SAMPLES)
					index = 0;
			}

			glBegin(GL_QUADS);
			for (int band = 0; band < 6; ++band)
			{
				glColor4fv(&band_info[band].r);
				float x = 0;
				float dx = 640.0f / NUM_SAMPLES;
				for (int i = 0; i < NUM_SAMPLES; i++)
				{
					glVertex3f(x, sample_y[band][i], 0);
					glVertex3f(x+dx, sample_y[band][i], 0);
					glVertex3f(x+dx, sample_y[band+1][i], 0);
					glVertex3f(x, sample_y[band+1][i], 0);
					x += dx;
				}
			}
			glEnd();

			// reset camera transform
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();

			// restore blend mode
			glPopAttrib();
		}
#endif

#ifdef PRINT_PERFORMANCE_DETAILS
		if (PROFILER_OUTPUTPRINT)
		{
			DebugPrint("C=%d S=%d P=%d U=%d R=%d D=%d\n",
				1000000 * control_time[profile_index] / perf_freq.QuadPart,
				1000000 * simulate_time[profile_index] / perf_freq.QuadPart,
				1000000 * collide_time[profile_index] / perf_freq.QuadPart,
				1000000 * update_time[profile_index] / perf_freq.QuadPart,
				1000000 * render_time[profile_index] / perf_freq.QuadPart,
				1000000 * display_time[profile_index] / perf_freq.QuadPart);
		}
#endif
#endif

		/* Render our console */
		OGLCONSOLE_Draw();

#ifdef GET_PERFORMANCE_DETAILS
		if (OPENGL_SWAPCONTROL)
#endif
		// wait for rendering to finish
		glFinish();

		// show the screen
		SDL_GL_SwapBuffers();

		// clear device reset flag
		wasreset = false;
	}
	while( !quit );

	DebugPrint("Quitting...\n");

	if (record)
	{
		// save input log
		inputlog.SaveFile();
	}

	// stop audio
	SDL_PauseAudio(1);

	// clear all databases
	Database::Cleanup();

	// collidable done
	Collidable::WorldDone();
}
