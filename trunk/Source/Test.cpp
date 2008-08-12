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
#include "Overlay.h"

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

// frame rate indicator
bool FRAMERATE_OUTPUTSCREEN = false;
bool FRAMERATE_OUTPUTPRINT = false;

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

// pause state
bool paused = false;
bool singlestep = false;

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
#define PRINT_PERFORMANCE_FRAMERATE
#define DRAW_PERFORMANCE_FRAMERATE
//#define PRINT_SIMULATION_TIMER
#define TRACE_OPENGL_ATTRIBUTES

// input system
Input input;

// frame values (HACK)
float frame_time;
float frame_turns;

// simulation values (HACK)
float sim_rate = float(SIMULATION_RATE);
float sim_step = 1.0f / sim_rate;
unsigned int sim_turn = 0;
float sim_fraction = 1.0f;

// listener position (HACK)
Vector2 listenerpos;

// reticule handle (HACK)
GLuint reticule_handle;
GLuint score_handle;
GLuint lives_handle;

// forward declaration
int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );
void RunState();
void EnterShellState();
void ExitShellState();
void EnterPlayState();
void ExitPlayState();

// game state machine (HACK)
enum GameStateType
{
	STATE_NONE,
	STATE_SHELL,
	STATE_PLAY,
	STATE_QUIT,
	NUM_GAME_STATES
};
GameStateType curgamestate = STATE_NONE;
GameStateType setgamestate = STATE_SHELL;

struct GameState
{
	fastdelegate::FastDelegate<void ()> OnEnter;
	fastdelegate::FastDelegate<void ()> OnUpdate;
	fastdelegate::FastDelegate<void ()> OnExit;
};

GameState gamestates[NUM_GAME_STATES] = 
{
	{ NULL, NULL, NULL },
	{ EnterShellState, RunState, ExitShellState },
	{ EnterPlayState, RunState, ExitPlayState },
	{ NULL, NULL, NULL },
};


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

	// set window option
	SDL_WM_SetCaption( "Shmup!", NULL );

    /* Initialize OGLCONSOLE */                                                                      
    console = OGLCONSOLE_Create();                                                                             
	OGLCONSOLE_EditConsole(console);
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


bool init_Input(const char *config)
{
	// clear existing bindings
	input.Clear();

	// load input binding file
	DebugPrint("Input %s\n", config);
	TiXmlDocument document(config);
	document.LoadFile();

	// process child elements of the root
	if (const TiXmlElement *root = document.FirstChildElement("input"))
	{
		input.Configure(root);
		return true;
	}

	return false;
}

bool init_Level(const char *config)
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
	{
		ProcessWorldItems(root);

		// get the reticule draw list (HACK)
		reticule_handle = Database::drawlist.Get(0x170e4c58 /* "reticule" */);

		// play the startup sound (HACK)
		PlaySound(0x94326baa /* "startup" */);

		return true;
	}

	// clear the reticule draw list (HACK)
	reticule_handle = 0;

	// show the mouse cursor
	SDL_ShowCursor(SDL_ENABLE);

	return false;

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

	case 0x24ce5450 /* "frameratescreen" */:
		return ProcessCommandBool(FRAMERATE_OUTPUTSCREEN, aParam, aCount, NULL, "frameratescreen: %d\n");

	case 0x55cfbc33 /* "framerateprint" */:
		return ProcessCommandBool(FRAMERATE_OUTPUTPRINT, aParam, aCount, NULL, "framerateprint: %d\n");

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
	
	do
	{
		if (setgamestate != curgamestate)
		{
			if (gamestates[curgamestate].OnExit)
				gamestates[curgamestate].OnExit();
			curgamestate = setgamestate;
			if (gamestates[curgamestate].OnEnter)
				gamestates[curgamestate].OnEnter();
		}
		if (gamestates[curgamestate].OnUpdate)
			gamestates[curgamestate].OnUpdate();
	}
	while(curgamestate != STATE_QUIT);

	// clean up
	clean_up();

	// done
	return 0;
}


//
// SHELL STATE
//

// convert HSV [0..1] to RGB [0..1]
void HSV2RGB(float h, float s, float v, float &r, float &g, float &b)
{
#if 1
	// convert hue to index and fraction
	const int bits = 20;
	int scaled = (xs_FloorToInt(h * (1 << bits)) & ((1 << bits) - 1)) * 6;
	int i = scaled >> bits;
	float f = scaled * (1.0f / (1 << bits)) - i;

	// generate components
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);

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
	float R = std::min(std::max(Y + S * sin(theta), 0.0f), 1.0f);
	float G = std::min(std::max(Y - (27/53) * S * sin(theta) - (10/53) * S * cos(theta), 0.0f), 1.0f);
	float B = std::min(std::max(Y + S * cos(theta), 0.0f), 1.0f);
#endif
}

// draw title
void RenderShellTitle(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	glBegin(GL_QUADS);

	// title text bitmap
	static const char titlemap[][12*8+1] = 
	{
	//   123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678
		" ##  ##  ######  ####    ######   ####   ##  ##  ######  ##  ##  ######  ##  ##  #####   ###### ",
		" ##  ##    ##    ## ##   ##      ##  ##  ##  ##  ##      ### ##    ##    ##  ##  ##  ##  ##     ",
		" ##  ##    ##    ##  ##  #####   ##  ##  ##  ##  #####   ######    ##    ##  ##  ##  ##  #####  ",
		" ##  ##    ##    ##  ##  ##      ##  ##  ##  ##  ##      ######    ##    ##  ##  #####   ##     ",
		"  ####     ##    ## ##   ##      ##  ##   ####   ##      ## ###    ##    ##  ##  ## ##   ##     ",
		"   ##    ######  ####    ######   ####     ##    ######  ##  ##    ##    ######  ##  ##  ###### ",
	};

	// title drawing properties
	static const float titlew = 6;
	static const float titleh = 4;
	static const float titlex = 320 - titlew * 0.5f * SDL_arraysize(titlemap[0]);
	static const float titley = 16;
	static const float titlez = 0;

	// title bar alphas
	static float baralpha[SDL_arraysize(titlemap)] = { 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f };

	// draw title bar
	for (int row = 0; row < SDL_arraysize(titlemap); ++row)
	{
		float y0 = titley + row * titleh, y1 = y0 + titleh;

		glColor4f(0.3f, 0.3f, 0.3f, baralpha[row]);
		glVertex2f(0, y0);
		glVertex2f(640, y0);
		glVertex2f(640, y1);
		glVertex2f(0, y1);
	}

	// draw title border
	for (int row = 0; row < SDL_arraysize(titlemap); ++row)
	{
		float x0 = titlex, x1 = titlex, y0 = titley + row * titleh, y1 = y0 + titleh;

		for (int col = 0; col < SDL_arraysize(titlemap[row])-1; ++col)
		{
			if (titlemap[row][col] == ' ')
			{
				if (x1 > x0)
				{
					float R, G, B;
					HSV2RGB((sim_turn + 8 * (col >> 3) + 4 * row) / 256.0f + 0.5f, 1.0f, 0.75f, R, G, B);
					glColor4f(R, G, B, 1.0f);
					glVertex2f(x0 - 2, y0 - 2);
					glVertex2f(x1 + 2, y0 - 2);
					glVertex2f(x1 + 2, y1 + 2);
					glVertex2f(x0 - 2, y1 + 2);
				}
				x0 = titlex + col * titlew + titlew;
			}
			else
			{
				x1 = titlex + col * titlew + titlew;
			}
		}
	}

	// draw title body
	for (int row = 0; row < SDL_arraysize(titlemap); ++row)
	{
		float x0 = titlex, x1 = titlex, y0 = titley + row * titleh, y1 = y0 + titleh;
		for (int col = 0; col < SDL_arraysize(titlemap[row]); ++col)
		{
			if (titlemap[row][col] == ' ')
			{
				if (x1 > x0)
				{
					float R, G, B;
					HSV2RGB((sim_turn + 8 * (col >> 3) + 4 * row) / 256.0f, 1.0f, 1.0f, R, G, B);
					glColor4f(R, G, B, 1.0f);
					glVertex2f(x0, y0);
					glVertex2f(x1, y0);
					glVertex2f(x1, y1);
					glVertex2f(x0, y1);
				}
				x0 = titlex + col * titlew + titlew;
			}
			else
			{
				x1 = titlex + col * titlew + titlew;
			}
		}
	}

	glEnd();
}

// draw options
void RenderShellOptions(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	static const int optioncount = 3;
	static char * const optiontext[optioncount] =
	{
		"START",
		"OPTIONS",
		"QUIT" 
	};
	static const float optionw = 32;
	static const float optionh = -24;
	static const float optionbuttonx[optioncount] =
	{
		320 - 160,
		320 - 160,
		320 - 160,
	};
	static const float optionbuttonw[optioncount] =
	{
		320,
		320,
		320,
	};
	static const float optiontextx[optioncount] =
	{
		optionbuttonx[0] + 0.5f * (optionbuttonw[0] - optionw * strlen(optiontext[0])), 
		optionbuttonx[1] + 0.5f * (optionbuttonw[1] - optionw * strlen(optiontext[1])), 
		optionbuttonx[2] + 0.5f * (optionbuttonw[2] - optionw * strlen(optiontext[2]))
	};
	static const float optionbuttony[optioncount] =
	{
		200 + 80 * 0,
		200 + 80 * 1,
		200 + 80 * 2
	};
	static const float optionbuttonh[optioncount] =
	{
		64,
		64,
		64
	};
	static const float optiontexty[optioncount] = 
	{
		optionbuttony[0] + 0.5f * (optionbuttonh[0] - optionh),
		optionbuttony[1] + 0.5f * (optionbuttonh[1] - optionh),
		optionbuttony[2] + 0.5f * (optionbuttonh[2] - optionh),
	};
	static const float optionz = 0;

	// palette
	enum ButtonState
	{
		BUTTON_NORMAL = 0,
		BUTTON_SELECTED = 1 << 0,
		BUTTON_ROLLOVER = 1 << 1,
	};
	static const float optionbackcolor[4][4] =
	{
		{ 0.1f, 0.1f, 0.1f, 0.5f },
		{ 0.1f, 0.3f, 1.0f, 0.5f },
		{ 0.3f, 0.3f, 0.3f, 0.5f },
		{ 0.1f, 0.7f, 1.0f, 0.5f },
	};
	static const float optionbordercolor[4][4] =
	{
		{ 0.3f, 0.3f, 0.3f, 1.0f },
		{ 0.3f, 0.3f, 0.3f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
	};
	static const float optiontextcolor[4][2][4] =
	{
		{ { 0.1f, 0.6f, 1.0f, 1.0f }, { 0.1f, 0.6f, 1.0f, 1.0f } },
		{ { 1.0f, 0.9f, 0.1f, 1.0f }, { 1.0f, 0.9f, 0.1f, 1.0f } },
		{ { 0.7f, 0.7f, 0.7f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ { 1.0f, 0.9f, 0.1f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	};

	// cursor position
	float cursor_x = 320 - 240 * input.value[Input::AIM_HORIZONTAL];
	float cursor_y = 240 - 240 * input.value[Input::AIM_VERTICAL];

	// mouse rollover
	int optionstate[optioncount];
	for (int i = 0; i < optioncount; ++i)
	{
		optionstate[i] = BUTTON_NORMAL;
		if (cursor_x >= optionbuttonx[i] && cursor_x <= optionbuttonx[i] + optionbuttonw[i] &&
			cursor_y >= optionbuttony[i] && cursor_y <= optionbuttony[i] + optionbuttonh[i])
		{
			optionstate[i] |= BUTTON_ROLLOVER;
			if (input[Input::FIRE_PRIMARY])
			{
				optionstate[i] |= BUTTON_SELECTED;

				switch (i)
				{
				case 0:
					setgamestate = STATE_PLAY;
					break;

				case 2:
					setgamestate = STATE_QUIT;
					break;

				default:
					break;
				}
			}
		}
	}

	// draw option backgrounds
	glBegin(GL_QUADS);
	for (int i = 0; i < optioncount; ++i)
	{
		glColor4fv(optionbackcolor[optionstate[i]]);
		glVertex2f(optionbuttonx[i], optionbuttony[i]);
		glVertex2f(optionbuttonx[i] + optionbuttonw[i], optionbuttony[i]);
		glVertex2f(optionbuttonx[i] + optionbuttonw[i], optionbuttony[i] + optionbuttonh[i]);
		glVertex2f(optionbuttonx[i], optionbuttony[i] + optionbuttonh[i]);
	}
	glEnd();

	// draw option text
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

	glBegin(GL_QUADS);

	for (int i = 0; i < optioncount; ++i)
	{
		glColor4fv(optionbordercolor[optionstate[i]]);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i] - 2, optiontexty[i] - 2, optionw, optionh, optionz);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i]    , optiontexty[i] - 2, optionw, optionh, optionz);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i] + 2, optiontexty[i] - 2, optionw, optionh, optionz);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i] - 2, optiontexty[i]    , optionw, optionh, optionz);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i] + 2, optiontexty[i]    , optionw, optionh, optionz);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i] - 2, optiontexty[i] + 2, optionw, optionh, optionz);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i]    , optiontexty[i] + 2, optionw, optionh, optionz);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i] + 2, optiontexty[i] + 2, optionw, optionh, optionz);

		float color[4];
		float interp = ((sim_turn & 16) ? 16 - (sim_turn & 15) : (sim_turn & 15)) / 16.0f;
		for (int c = 0; c < 4; c++)
			color[c] = Lerp(optiontextcolor[optionstate[i]][0][c], optiontextcolor[optionstate[i]][1][c], interp);
		glColor4fv(color);
		OGLCONSOLE_DrawString(optiontext[i], optiontextx[i], optiontexty[i], optionw, optionh, optionz);
	}

	glEnd();

	glDisable(GL_TEXTURE_2D);
}

// draw cursor
void RenderShellCursor(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	// cursor position
	float cursor_x = 320 - 240 * input.value[Input::AIM_HORIZONTAL];
	float cursor_y = 240 - 240 * input.value[Input::AIM_VERTICAL];

	// draw reticule (HACK)
	glPushMatrix();
	glTranslatef(cursor_x, cursor_y, 0.0f);
	glCallList(reticule_handle);
	glPopMatrix();
}

// enter shell state
void EnterShellState()
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

	// reset simulation timer
	sim_rate = float(SIMULATION_RATE);
	sim_step = 1.0f / sim_rate;
	sim_turn = 0;
	sim_fraction = 1.0f;

	// collidable initialization
	Collidable::WorldInit();

	// input binding
	init_Input(INPUT_CONFIG.c_str());

	// level configuration
	init_Level("shell.xml");

	// start audio
	SDL_PauseAudio(0);

	// create title overlay
	Overlay *title = new Overlay(0x9865b509 /* "title" */);
	Database::overlay.Put(0x9865b509 /* "title" */, title);
	title->SetAction(Overlay::Action(RenderShellTitle));
	title->Show();

	// create options overlay
	Overlay *options = new Overlay(0xef286ca5 /* "options" */);
	Database::overlay.Put(0xef286ca5 /* "options" */, options);
	options->SetAction(Overlay::Action(RenderShellOptions));
	options->Show();

	// create cursor overlay
	Overlay *cursor = new Overlay(0xe336320f /* "cursor" */);
	Database::overlay.Put(0xe336320f /* "cursor" */, cursor);
	cursor->SetAction(Overlay::Action(RenderShellCursor));
	cursor->Show();

	// set to runtime mode
	runtime = true;
}

void ExitShellState()
{
	// stop audio
	SDL_PauseAudio(1);

	// stop any startup sound (HACK)
	StopSound(0x94326baa /* "startup" */);

	// clear overlays
	delete Database::overlay.Get(0x9865b509 /* "title" */);
	Database::overlay.Delete(0x9865b509 /* "title" */);
	delete Database::overlay.Get(0xef286ca5 /* "options" */);
	Database::overlay.Delete(0xef286ca5 /* "options" */);
	delete Database::overlay.Get(0xe336320f /* "cursor" */);
	Database::overlay.Delete(0xe336320f /* "cursor" */);

	// clear all databases
	Database::Cleanup();

	// collidable done
	Collidable::WorldDone();

	// set to non-runtime mode
	runtime = false;
}


//
// PLAY STATE
//

// drain values
static const float DRAIN_DELAY = 1.0f;
static const float DRAIN_RATE = 0.5f;

// flash values
static const int MAX_FLASH = 16;
static const float FLASH_RATE = 2.0f;

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

class PlayerHUD : public Updatable, public Overlay
{
public:
	// fill values
	float fill;

	// drain values
	float drain;
	float draindelay;

	// flash values
	struct Flash
	{
		float left;
		float right;
		float fade;
	};
	Flash flash[MAX_FLASH];
	int flashcount;

	// camera values
	Vector2 trackpos[2];
	Vector2 trackaim;

	// reticule values
	Vector2 aimpos[2];

public:
	PlayerHUD(unsigned int aPlayerId);

	void Update(float aStep);
	void Render(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle);
};

PlayerHUD::PlayerHUD(unsigned int aPlayerId = 0)
	: Updatable(aPlayerId)
	, Overlay(aPlayerId)
	, fill(0)
	, drain(0)
	, draindelay(0)
	, flashcount(0)
{
	trackpos[0] = Vector2(0, 0);
	trackpos[1] = Vector2(0, 0);
	trackaim = Vector2(0, 0);
	aimpos[0] = Vector2(0, 0);
	aimpos[1] = Vector2(0, 0);

	Updatable::SetAction(Updatable::Action(this, &PlayerHUD::Update));
	Overlay::SetAction(Overlay::Action(this, &PlayerHUD::Render));
}

void PlayerHUD::Update(float aStep)
{
	// get the player
	Player *player = Database::player.Get(Updatable::mId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;
	if (!id)
		return;

	// get the entity
	Entity *entity = Database::entity.Get(id);
	if (!entity)
		return;

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
		if (Database::ship.Get(id))
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

void PlayerHUD::Render(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;

	// draw player health (HACK)
	float health = 0.0f;
	Damagable *damagable = Database::damagable.Get(id);
	if (damagable)
	{
		// get health ratio
		const DamagableTemplate &damagabletemplate = Database::damagabletemplate.Get(id);
		health = damagable->GetHealth() / damagabletemplate.mHealth;
	}

	// if health is greater than the gauge fill...
	if (fill < health - FLT_EPSILON)
	{
		// raise the fill
		fill = health;
		if (drain < fill)
			drain = fill;
	}
	// else if health is lower than the gauge fill...
	else if (fill > health + FLT_EPSILON)
	{
		// add a flash
		if (flashcount == MAX_FLASH)
			--flashcount;
		for (int i = flashcount; i > 0; --i)
			flash[i] = flash[i-1];
		Flash &flashinfo = flash[0];
		flashinfo.left = health;
		flashinfo.right = fill;
		flashinfo.fade = 1.0f;
		++flashcount;

		// lower the fill
		fill = health;

		// reset the drain delay
		draindelay = DRAIN_DELAY;
	}

	// update pulse
	static float pulsetimer = 0.0f;
	pulsetimer += frame_time * (1.0f + (1.0f - health) * (1.0f - health) * 4.0f);
	while (pulsetimer >= 1.0f)
		pulsetimer -= 1.0f;
	float pulse = sinf(pulsetimer * float(M_PI));
	pulse *= pulse;
	pulse *= pulse;
	pulse *= pulse;

	// set color based on health and pulse

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
	glVertex2f(8 + 100 * fill, 8);
	glVertex2f(8 + 100 * drain, 8);
	glVertex2f(8 + 100 * drain, 16);
	glVertex2f(8 + 100 * fill, 16);

	// flash
	for (int i = 0; i < flashcount; ++i)
	{
		Flash &flashinfo = flash[i];
		glColor4f(1.0f, 1.0f, 1.0f, flashinfo.fade);
		glVertex2f(8 + 100 * flashinfo.left, 8 - 2 * flashinfo.fade);
		glVertex2f(8 + 100 * flashinfo.right, 8 - 2 * flashinfo.fade);
		glVertex2f(8 + 100 * flashinfo.right, 16 + 2 * flashinfo.fade);
		glVertex2f(8 + 100 * flashinfo.left, 16 + 2 * flashinfo.fade);
	}

	// fill gauge
	glColor4fv(fillcolor);
	glVertex2f(8, 8);
	glVertex2f(8 + 100 * fill, 8);
	glVertex2f(8 + 100 * fill, 16);
	glVertex2f(8, 16);

	glEnd();

	// if the drain delay elapsed...
	draindelay -= frame_time;
	if (draindelay <= 0)
	{
		// update drain
		drain -= DRAIN_RATE * frame_time;
		if (drain < fill)
			drain = fill;
	}

	// count down flash timers
	for (int i = 0; i < flashcount; ++i)
	{
		Flash &flashinfo = flash[i];
		flashinfo.fade -= FLASH_RATE * frame_time;
		if (flashinfo.fade <= 0.0f)
		{
			flashcount = i;
			break;
		}
	}

	// get player score
	static int cur_score = -1;
	int new_score = player->mScore;

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
	int new_lives = player->mLives;
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
		float x = 320 - 240 * Lerp(aimpos[0].x, aimpos[1].x, sim_fraction);
		float y = 240 - 240 * Lerp(aimpos[0].y, aimpos[1].y, sim_fraction);

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
}

namespace Database
{
	Typed<PlayerHUD> playerhud(0x8e522e29 /* "playerhud" */);
}

// player join
void PlayerJoinListener(unsigned int aId)
{
	// create player hud overlay
	PlayerHUD *playerhud = new (Database::playerhud.Alloc(aId)) PlayerHUD(aId);
	playerhud->Activate();
	playerhud->Show();
}

// player quit
void PlayerQuitListener(unsigned int aId)
{
	// remove player hud overlay
	Database::playerhud.Delete(aId);
}

// enter play state
void EnterPlayState()
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

	// reset simulation timer
	sim_rate = float(SIMULATION_RATE);
	sim_step = 1.0f / sim_rate;
	sim_turn = 0;
	sim_fraction = 1.0f;

	// collidable initialization
	Collidable::WorldInit();

	// input binding
	init_Input(INPUT_CONFIG.c_str());

	// level configuration
	if (!init_Level(LEVEL_CONFIG.c_str()))
		setgamestate = STATE_SHELL;

	// start audio
	SDL_PauseAudio(0);

	// add a join listener
	Database::playerjoin.Put(0xe28d61c6 /* "hud" */, PlayerJoinListener);

	// add a quit listener
	Database::playerquit.Put(0xe28d61c6 /* "hud" */, PlayerQuitListener);

	// allocate score draw list
	score_handle = glGenLists(1);

	// allocate lives draw list
	lives_handle = glGenLists(1);

	// set to runtime mode
	runtime = true;

	DebugPrint("Simulating at %dHz (x%f)\n", SIMULATION_RATE, TIME_SCALE);
}

// run play state
// exit play state
void ExitPlayState()
{
	DebugPrint("Quitting...\n");

	// stop audio
	SDL_PauseAudio(1);

	// stop any startup sound (HACK)
	StopSound(0x94326baa /* "startup" */);

	// clear all databases
	Database::Cleanup();

	// collidable done
	Collidable::WorldDone();

	// set to non-runtime mode
	runtime = false;
}

// common run state
void RunState()
{
	// last ticks
	unsigned int ticks = SDL_GetTicks();

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
	LONGLONG total_time[NUM_SAMPLES] = { 0 };
	int profile_index = -1;

	LARGE_INTEGER perf_lastframe;
	QueryPerformanceCounter(&perf_lastframe);
#endif

#ifdef COLLECT_DEBUG_DRAW
	// create a new draw list
	GLuint debugdraw = glGenLists(1);
#endif

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
		total_time[profile_index] = 0;
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
					setgamestate = STATE_QUIT;
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
					SDL_ShowCursor(paused || !reticule_handle ? SDL_ENABLE : SDL_DISABLE);
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
				setgamestate = STATE_QUIT;
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

		// frame time and turns
		if (singlestep)
		{
			singlestep = false;

			// advance 1/60th of a second
			frame_time = TIME_SCALE / 60.0f;
			frame_turns = frame_time * sim_rate;
		}
		else if (paused)
		{
			// freeze time
			frame_time = 0.0f;
			frame_turns = 0.0f;

			// set turn counter to almost reach a new turn
			sim_fraction = 0.99609375f;
		}
		else if (FIXED_STEP)
		{
			// advance one simulation step
			frame_time = TIME_SCALE * sim_step;
			frame_turns = TIME_SCALE;
		}
		else
		{
			// advance by frame time
			frame_time = delta * TIME_SCALE / 1000.0f;
			frame_turns = frame_time * sim_rate;
		}

		// turns per motion-blur step
		float step_turns = std::min(TIME_SCALE * MOTIONBLUR_TIME, sim_step) / MOTIONBLUR_STEPS * sim_rate;

		// advance to beginning of motion blur steps
		sim_fraction += frame_turns;
		sim_fraction -= MOTIONBLUR_STEPS * step_turns;

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
			sim_fraction += step_turns;

			// while simulation turns to run...
			while (sim_fraction >= 1.0f)
			{
#ifdef COLLECT_DEBUG_DRAW
				// collect any debug draw
				glNewList(debugdraw, GL_COMPILE);
#endif

				// deduct a turn
				sim_fraction -= 1.0f;
				
				// update database
				Database::Update();

				if (playback)
				{
					// quit if out of turns
					if (!inputlognext)
					{
						setgamestate = STATE_SHELL;
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

				// step inputs for next turn
				input.Step();

				// advance the turn counter
				++sim_turn;

#ifdef COLLECT_DEBUG_DRAW
				// finish the draw list
				glEndList();
#endif
			}

#ifdef PRINT_SIMULATION_TIMER
			DebugPrint("delta=%d ticks=%d sim_t=%f\n", delta, ticks, sim_fraction);
#endif

#ifdef GET_PERFORMANCE_DETAILS
			LARGE_INTEGER perf_count0;
			QueryPerformanceCounter(&perf_count0);
#endif

			// RENDERING PHASE

			// push camera transform
			glPushMatrix();

			// get the first player hud overlay (HACK)
			Database::Typed<PlayerHUD>::Iterator playerhuditor(&Database::playerhud);
			const PlayerHUD &playerhud = playerhuditor.GetValue();

			// get interpolated track position
			Vector2 viewpos(Lerp(playerhud.trackpos[0].x, playerhud.trackpos[1].x, sim_fraction), Lerp(playerhud.trackpos[0].y, playerhud.trackpos[1].y, sim_fraction));

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
			Renderable::RenderAll(view);

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
		}

		// switch blend mode
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

#ifdef COLLECT_DEBUG_DRAW
		// push camera transform
		glPushMatrix();

		// get the first player hud overlay (HACK)
		Database::Typed<PlayerHUD>::Iterator playerhuditor(&Database::playerhud);
		const PlayerHUD &playerhud = playerhuditor.GetValue();

		// get interpolated track position
		Vector2 viewpos(Lerp(playerhud.trackpos[0].x, playerhud.trackpos[1].x, sim_fraction), Lerp(playerhud.trackpos[0].y, playerhud.trackpos[1].y, sim_fraction));

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

		// render all overlays
		Overlay::RenderAll();

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

		// update frame timer
		LARGE_INTEGER perf_thisframe;
		QueryPerformanceCounter(&perf_thisframe);
		total_time[profile_index] = perf_thisframe.QuadPart - perf_lastframe.QuadPart;
		perf_lastframe = perf_thisframe;

#if defined(PRINT_PERFORMANCE_FRAMERATE) || defined(DRAW_PERFORMANCE_FRAMERATE)
		if (FRAMERATE_OUTPUTSCREEN || FRAMERATE_OUTPUTPRINT)
		{
			// compute minimum, maximum, and average frame times over the past second
			LONGLONG total_min = LLONG_MAX;
			LONGLONG total_max = LLONG_MIN;
			LONGLONG total_sum = 0;
			LONGLONG total_samples = 0;
			int i = profile_index;
			do
			{
				total_min = std::min(total_min, total_time[i]);
				total_max = std::max(total_max, total_time[i]);
				total_sum += total_time[i];
				++total_samples;
				i = (i > 0) ? i - 1 : NUM_SAMPLES - 1;
			}
			while (total_sum <= perf_freq.QuadPart && i != profile_index);
			total_sum /= total_samples;

			// compute frame rates
			double rate_max = (double)perf_freq.QuadPart / total_min;
			double rate_avg = (double)perf_freq.QuadPart / total_sum;
			double rate_min = (double)perf_freq.QuadPart / total_max;

#if defined(DRAW_PERFORMANCE_FRAMERATE)
			if (FRAMERATE_OUTPUTSCREEN)
			{
				// draw frame rate indicator
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

				glBegin(GL_QUADS);

				char fps[16];
				sprintf(fps, "%.2f max", rate_max);
				glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
				OGLCONSOLE_DrawString(fps, 640 - 16 - 8 * strlen(fps), 16, 8, -8, 0);
				sprintf(fps, "%.2f avg", rate_avg);
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				OGLCONSOLE_DrawString(fps, 640 - 16 - 8 * strlen(fps), 24, 8, -8, 0);
				sprintf(fps, "%.2f min", rate_min);
				glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
				OGLCONSOLE_DrawString(fps, 640 - 16 - 8 * strlen(fps), 32, 8, -8, 0);

				glEnd();

				glDisable(GL_TEXTURE_2D);
			}
#endif

#if defined(PRINT_PERFORMANCE_FRAMERATE)
			if (FRAMERATE_OUTPUTPRINT)
			{
				DebugPrint("%.2f<%.2f<%.2f\n", rate_min, rate_avg, rate_max);
			}
#endif
		}
#endif
#endif

		// reset camera transform
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		// restore blend mode
		glPopAttrib();

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
	while( setgamestate == curgamestate );

	if (record)
	{
		// save input log
		inputlog.SaveFile();
	}
}
