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
#include "Resource.h"
#include "Variable.h"

#ifdef USE_PATHING
#include "Pathing.h"
#endif

#include <malloc.h>

#include <algorithm>


// screen attributes
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
int SCREEN_DEPTH = 0;
#ifdef _DEBUG
bool SCREEN_FULLSCREEN = false;
#else
bool SCREEN_FULLSCREEN = true;
#endif

// view attributes
float VIEW_SIZE = 320;
float VIEW_AIM = 80;
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
bool escape = false;

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

// random number seed (HACK)
unsigned long randlongseed = 0;

// camera position (HACK)
Vector2 camerapos[2];

// listener position (HACK)
Vector2 listenerpos;

// reticule handle (HACK)
GLuint reticule_handle;
GLuint score_handle;
GLuint lives_handle;
GLuint ammo_handle;

// forward declaration
int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );
void RunState();
void EnterShellState();
void ExitShellState();
void EnterPlayState();
void ExitPlayState();
void ReloadState();

// game state machine (HACK)
enum GameStateType
{
	STATE_NONE,
	STATE_SHELL,
	STATE_PLAY,
	STATE_RELOAD,
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
	{ NULL, ReloadState, NULL },
	{ NULL, NULL, NULL }
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

bool ReadPreferences(const char *config)
{
	// load input binding file
	DebugPrint("Preferences %s\n", config);
	TiXmlDocument document(config);
	document.LoadFile();

	// process child elements of the root
	if (const TiXmlElement *root = document.FirstChildElement("preferences"))
	{
		for (const TiXmlElement *element = root->FirstChildElement(); element != NULL; element = element->NextSiblingElement())
		{
			switch (Hash(element->Value()))
			{
			case 0x1d215c8f /* "resolution" */:
				element->QueryIntAttribute("width", &SCREEN_WIDTH);
				element->QueryIntAttribute("height", &SCREEN_HEIGHT);
				break;

			case 0x5032fb58 /* "fullscreen" */:
				{
					int enable = SCREEN_FULLSCREEN;
					element->QueryIntAttribute("enable", &enable);
					SCREEN_FULLSCREEN = enable != 0;
				}
				break;

			case 0x423e6b0c /* "verticalsync" */:
				{
					int enable = OPENGL_SWAPCONTROL;
					element->QueryIntAttribute("enable", &enable);
					OPENGL_SWAPCONTROL = enable != 0;
				}
				break;

			case 0x47d0f228 /* "multisample" */:
				element->QueryIntAttribute("samples", &OPENGL_MULTISAMPLE);
				break;

			case 0xf744f3b2 /* "motionblur" */:
				element->QueryIntAttribute("steps", &MOTIONBLUR_STEPS);
				if (element->QueryFloatAttribute("strength", &MOTIONBLUR_TIME) == TIXML_SUCCESS)
					MOTIONBLUR_TIME /= 6000;
				break;

			case 0x0e0d9594 /* "sound" */:
				element->QueryIntAttribute("channels", &SOUND_CHANNELS);
				if (element->QueryFloatAttribute("volume", &SOUND_VOLUME) == TIXML_SUCCESS)
					SOUND_VOLUME /= 100;
				break;
			}
		}
		return true;
	}

	return false;
}

bool WritePreferences(const char *config)
{
	// load input binding file
	DebugPrint("Preferences %s\n", config);
	TiXmlDocument document(config);

	TiXmlDeclaration * declaration = new TiXmlDeclaration( "1.0", "", "" );
	document.LinkEndChild(declaration);

	TiXmlElement *preferences = new TiXmlElement("preferences");
	document.LinkEndChild(preferences);

	TiXmlElement *resolution = new TiXmlElement("resolution");
	resolution->SetAttribute("width", SCREEN_WIDTH);
	resolution->SetAttribute("height", SCREEN_HEIGHT);
	preferences->LinkEndChild(resolution);

	TiXmlElement *fullscreen = new TiXmlElement("fullscreen");
	fullscreen->SetAttribute("enable", SCREEN_FULLSCREEN);
	preferences->LinkEndChild(fullscreen);

	TiXmlElement *verticalsync = new TiXmlElement("verticalsync");
	verticalsync->SetAttribute("enable", OPENGL_SWAPCONTROL);
	preferences->LinkEndChild(verticalsync);

	TiXmlElement *multisample = new TiXmlElement("multisample");
	multisample->SetAttribute("samples", OPENGL_MULTISAMPLE);
	preferences->LinkEndChild(multisample);

	TiXmlElement *motionblur = new TiXmlElement("motionblur");
	motionblur->SetAttribute("steps", MOTIONBLUR_STEPS);
	motionblur->SetAttribute("strength", xs_RoundToInt(MOTIONBLUR_TIME*6000));
	preferences->LinkEndChild(motionblur);

	TiXmlElement *sound = new TiXmlElement("sound");
	sound->SetAttribute("channels", SOUND_CHANNELS);
	sound->SetAttribute("volume", xs_RoundToInt(SOUND_VOLUME*100));
	preferences->LinkEndChild(sound);

	document.SaveFile();
	return true;
}


bool InitInput(const char *config)
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

bool InitLevel(const char *config)
{
	// load level data file
	DebugPrint("Level %s\n", config);
	TiXmlDocument document(config);
	document.LoadFile();

	// if the document has a world section...
	if (const TiXmlElement *root = document.FirstChildElement("world"))
	{
		// set up the collidable world
		float aMinX = -2048, aMinY = -2048, aMaxX = 2048, aMaxY = 2048;
		root->QueryFloatAttribute("xmin", &aMinX);
		root->QueryFloatAttribute("ymin", &aMinY);
		root->QueryFloatAttribute("xmax", &aMaxX);
		root->QueryFloatAttribute("ymax", &aMaxY);
		Collidable::WorldInit(aMinX, aMinY, aMaxX, aMaxY);

		// process child elements of world
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
		InitInput(INPUT_CONFIG.c_str());
}
void InitLevelAction()
{
	if (runtime)
	{
		if (curgamestate == STATE_PLAY)
		{
			setgamestate = STATE_RELOAD;
		}
	}
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
	case 0x11e1fc01 /* "shell" */:
		setgamestate = STATE_SHELL;
		return 0;

	case 0xc2cbd863 /* "play" */:
		setgamestate = STATE_PLAY;
		return 0;

	case 0xed7cdd8c /* "reload" */:
		setgamestate = STATE_RELOAD;
		return 0;

	case 0x47878736 /* "quit" */:
		setgamestate = STATE_QUIT;
		return 0;

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

		// default to output debug
		DEBUGPRINT_OUTPUTDEBUG = true;
	}
#endif

	// read preferences
	ReadPreferences("preferences.xml");

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
// TURN ACTION QUEUE

// turn action
struct TurnAction
{
	unsigned int mTurn;
	float mFraction;
	fastdelegate::FastDelegate<void ()> mAction;

	TurnAction(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction)
		: mTurn(aTurn), mFraction(aFraction), mAction(aAction)
	{
	}
};

// compare turn actions
struct TurnActionCompare
{
	bool operator() (const TurnAction &a1, const TurnAction &a2)
	{
		float delta = (int)(a1.mTurn - a2.mTurn) + a1.mFraction - a2.mFraction;
		return (delta > 0.0f) || (delta == 0.0f && a1.mAction > a2.mAction);
	}
};
TurnActionCompare turnactioncompare;

// queued actions
std::deque<TurnAction> turnactions;

// queue a turn action
void OnTurn(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction)
{
	turnactions.push_back(TurnAction(aTurn, aFraction, aAction));
}

// perform actions for this turn
void DoTurn(void)
{
	while (!turnactions.empty())
	{
		std::make_heap(turnactions.begin(), turnactions.end(), turnactioncompare);
		const TurnAction &a = turnactions.front();

		if (int(sim_turn - a.mTurn) + sim_fraction - a.mFraction < 0.0f)
			break;

		(a.mAction)();
		turnactions.pop_front();
	}
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
	float R = Clamp(Y + S * sin(theta), 0.0f, 1.0f);
	float G = Clamp(Y - (27/53) * S * sin(theta) - (10/53) * S * cos(theta), 0.0f, 1.0f);
	float B = Clamp(Y + S * cos(theta), 0.0f, 1.0f);
#endif
}


// draw title
void RenderShellTitle(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	// title text bitmap
	static const char titlemap[][12*8+1] = 
	{
	//   123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678
		/*
		" 1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111 ",
		"                                                                                                ",
		"                                                                                                ",
		" 00  00  000000  00000   000000   0000   00  00  000000  00  00  000000  00  00  00000   000000 ",
		" 00  00    00    00  00  00      00  00  00  00  00      000 00    00    00  00  00  00  00     ",
		" 00  00    00    00  00  0000    00  00  00  00  0000    000000    00    00  00  00000   0000   ",
		"  0000     00    00  00  00      00  00   0000   00      00 000    00    00  00  00  00  00     ",
		"   00    000000  00000   000000   0000     00    000000  00  00    00     0000   00  00  000000 ",
		"                                                                                                ",
		"                                                                                                ",
		" 1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111 ",
		"                                                                                                ",
		"                                                                                                ",
		*/
		" 0000000000000    000000000000    000000000000   000000  000000  00000000000000  00000000000000 ",
		" 00000000000000  00000000000000  00000000000000  000000  000000  00000000000000  00000000000000 ",
		" 000000  000000  000000  000000  000000  000000  000000 000000   000000              000000     ",
		" 00000000000000  000000  000000  000000          000000000000    000000000           000000     ",
		" 0000000000000   000000  000000  000000          000000000000    000000000           000000     ",
		" 000000  000000  000000  000000  000000  000000  000000 000000   000000              000000     ",
		" 000000  000000  00000000000000  00000000000000  000000  000000  00000000000000      000000     ",
		" 000000  000000   000000000000    000000000000   000000  000000  00000000000000      000000     ",
		"                                                                                                ",
		"                                                                                                ",
		" 111111111111111111111    11111111111111111111   111111111    111111111  111111111111111111111  ",
		" 1111111111111111111111  1111111111111111111111  1111111111  1111111111  1111111111111111111111 ",
		" 1111111111111111111111  1111111111111111111111  1111111111111111111111  1111111111111111111111 ",
		" 1111111111  1111111111  1111111111  1111111111  1111111111111111111111  1111111111  1111111111 ",
		" 1111111111  1111111111  1111111111  1111111111  11111111 1111 11111111  1111111111  1111111111 ",
		" 111111111111111111111   1111111111  1111111111  111111111 11 111111111  111111111111111111111  ",
		" 111111111111111111111   1111111111  1111111111  1111111111  1111111111  111111111111111111111  ",
		" 1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111 ",
		" 1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111 ",
		" 1111111111111111111111  1111111111111111111111  1111111111  1111111111  1111111111111111111111 ",
		" 1111111111111111111111  1111111111111111111111  1111111111  1111111111  1111111111111111111111 ",
		" 111111111111111111111    11111111111111111111   1111111111  1111111111  111111111111111111111  ",
	};

	// border drawing properties
	static const float borderw = 2;
	static const float borderh = 2;

	// title drawing properties
	static const float titlew = 6;
	static const float titleh = 6;
	static const float titlex = 320 - titlew * 0.5f * (SDL_arraysize(titlemap[0]) - 1);
	static const float titley = 16;
	static const float titlez = 0;

	// title bar alphas
	static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f };

	glBegin(GL_QUADS);

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

	// border rectangles
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
		(1<<0), ((1<<0)|(1<<1)|(1<<2)), (1<<2),
		((1<<0)|(1<<3)|(1<<6)), (1 << 4), ((1<<2)|(1<<5)|(1<<8)),
		(1<<6), ((1<<6)|(1<<7)|(1<<8)), (1<<8)
	};

#if 1

//#define USE_TITLE_MIRROR_WATER_EFFECT
#ifdef USE_TITLE_MIRROR_WATER_EFFECT
	// mirror offset
	float mirrorscale = -0.75f;
	float titleheight = titleh * (SDL_arraysize(titlemap) + 1);
	float mirrortop = titley + titleheight + titleh * 2 + 4;
	float mirrorbottom = mirrortop - mirrorscale * titleheight;
	float mirroralphadelta = -0.375f / 32;
	float mirroralphastart = 0.375f - mirroralphadelta * mirrortop;
#endif

	// draw title body
	for (int row = -1; row < (int)SDL_arraysize(titlemap) + 1; ++row)
	{
		float y = titley + row * titleh;

		for (int col = -1; col < (int)SDL_arraysize(titlemap[0]); ++col)
		{
			float x = titlex + col * titlew;

			int phase = -1;
			int fill = 0;

			int c0 = std::max<int>(col - 1, 0);
			int c1 = std::min<int>(col + 1, SDL_arraysize(titlemap[0]) - 2);
			int r0 = std::max<int>(row - 1, 0);
			int r1 = std::min<int>(row + 1, SDL_arraysize(titlemap) - 1);

			for (int r = r0; r <= r1; ++r)
			{
				for (int c = c0; c <= c1; ++c)
				{
					if (titlemap[r][c] >= '0')
					{
						phase = titlemap[r][c] - '0';
						fill |= mask[(r - row + 1) * 3 + (c - col + 1)];
					}
				}
			}

			if (fill & (1<<4))
				fill = (1<<4);

			if (phase >= 0)
			{
				// get block color
				float R, G, B;
				float h = sim_turn / 1024.0f + col / 512.0f + 0.03125f * sinf(sim_turn / 64.0f + row / 4.0f + 4.0f * sinf(sim_turn / 64.0f + col / 8.0f + 0.5f * sinf(sim_turn / 64.0f + row / 4.0f)));
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
						glColor4f(R, G, B, 1.0f);
						glVertex2f(x0, y0);
						glVertex2f(x1, y0);
						glVertex2f(x1, y1);
						glVertex2f(x0, y1);

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
						// mirrored
						y0 = mirrorbottom + mirrorscale * y0 + 1.0f * sinf(sim_turn / 64.0f + y0 / 8.0f) + 3.0f * sinf(sim_turn / 128.0f + y0 / 32.0f);
						y1 = mirrorbottom + mirrorscale * y1 + 1.0f * sinf(sim_turn / 64.0f + y1 / 8.0f) + 3.0f * sinf(sim_turn / 128.0f + y1 / 32.0f);
						float a0 = mirroralphastart + mirroralphadelta * y0;
						float a1 = mirroralphastart + mirroralphadelta * y1;
						if (a0 > 0.0f || a1 > 0.0f)
						{
							a0 = std::max(a0, 0.0f);
							a1 = std::max(a1, 0.0f);
							float d0 = 1.0f * sinf(sim_turn / 32.0f + y0 / 4.0f);
							float d1 = 1.0f * sinf(sim_turn / 32.0f + y1 / 4.0f);
							glColor4f(R, G, B, a1 * a1);
							glVertex2f(x0 + d1, y1);
							glVertex2f(x1 + d1, y1);
							glColor4f(R, G, B, a0 * a0);
							glVertex2f(x1 + d0, y0);
							glVertex2f(x0 + d0, y0);
						}
#endif
					}
				}
			}
		}
	}

	glEnd();

#else
	// texture-based variant

	glEnd();

	glEnable(GL_TEXTURE_2D);

	static const int titletexwidth = 128;
	static const int titletexheight = 64;
	static const float titleborderu = float(borderw) / float(titlew * titletexwidth);
	static const float titleborderv = float(borderh) / float(titleh * titletexheight);

	static GLuint titletexture = 0;
	if (titletexture == 0)
	{
		glGenTextures(1, &titletexture);
		{int err=glGetError();if(err)DebugPrint("glGenTextures() error: %i\n",err);}
	}

	// bind title texture
	glBindTexture(GL_TEXTURE_2D, titletexture);
	{int err=glGetError();if(err)DebugPrint("glBindTexture() error: %i\n",err);}
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// generate texture data
	unsigned char texturedata[titletexheight][titletexwidth][3];
	for (int row = -1; row < (int)SDL_arraysize(titlemap) + 1; ++row)
	{
		for (int col = -1; col < (int)SDL_arraysize(titlemap[0]); ++col)
		{
			int phase = -1;
			int fill = 0;

			int c0 = std::max<int>(col - 1, 0);
			int c1 = std::min<int>(col + 1, SDL_arraysize(titlemap[0]) - 2);
			int r0 = std::max<int>(row - 1, 0);
			int r1 = std::min<int>(row + 1, SDL_arraysize(titlemap) - 1);

			for (int r = r0; r <= r1; ++r)
			{
				for (int c = c0; c <= c1; ++c)
				{
					if (titlemap[r][c] >= '0')
					{
						phase = titlemap[r][c] - '0';
						fill |= mask[(r - row + 1) * 3 + (c - col + 1)];
					}
				}
			}

			if (fill & (1<<4))
				fill = (1<<4);

			if (phase >= 0)
			{
				// get block color
				float R, G, B;
				float h = sim_turn / 1024.0f + col / 512.0f + 0.03125f * sinf(sim_turn / 64.0f + row / 4.0f + 4.0f * sinf(sim_turn / 64.0f + col / 8.0f + 0.5f * sinf(sim_turn / 64.0f + row / 4.0f)));
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
		}
	}

	// upload texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, titletexwidth, titletexheight, 0, GL_RGB, GL_UNSIGNED_BYTE, texturedata);
	{int err=glGetError();if(err)DebugPrint("glTexImage2D() error: %i\n",err);}

	// if no title slabs generated...
	static unsigned char titleslabmap[SDL_arraysize(titlemap)][SDL_arraysize(titlemap[0])-1];
	static int titleslab[255][4];
	static int titleslabcount = 0;
	if (titleslabcount == 0)
	{
		// initialize slab map
		memset(&titleslabmap[0][0], 0xFF, sizeof(titleslabmap));

		// generate title slabs
		for (int row = 0; row < SDL_arraysize(titlemap); ++row)
		{
			for (int col = 0; col < SDL_arraysize(titlemap[row])-1; ++col)
			{
				// skip empty spaces
				if (titlemap[row][col] == ' ')
					continue;

				// skip assigned spaces
				if (titleslabmap[row][col] != 0xFF)
					continue;

				// allocate a new index
				int index = titleslabcount++;

				// find horizontal extent
				int c0 = col;
				int c1 = SDL_arraysize(titlemap[row]) - 1;
				for (int c = c0; c < c1; ++c)
				{
					if ((titlemap[row][c] == ' ') || ((titleslabmap[row][c] != 0xFF) && (titleslabmap[row][c] != index)))
					{
						c1 = c;
						break;
					}
				}

				// find vertical extent
				int r0 = row;
				int r1 = SDL_arraysize(titlemap);
				for (int r = r0; r < r1; ++r)
				{
					for (int c = c0; c < c1; ++c)
					{
						if ((titlemap[r][c] == ' ') || ((titleslabmap[r][c] != 0xFF) && (titleslabmap[r][c] != index)))
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
						titleslabmap[r][c] = (unsigned char)index;
					}
				}

				assert(c0 < c1 && r0 < r1);

				// set slab extents
				titleslab[index][0] = c0;
				titleslab[index][1] = c1;
				titleslab[index][2] = r0;
				titleslab[index][3] = r1;

				// skip visited columns
				col = c1;
			}
		}
	}

	// draw title body
	glBegin(GL_QUADS);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// for each title slab...
	for (int i = 0; i < titleslabcount; ++i)
	{
		// get slab extents
		int c0 = titleslab[i][0];
		int c1 = titleslab[i][1];
		int r0 = titleslab[i][2];
		int r1 = titleslab[i][3];

		// generate texture extents
		float u0 = float(c0+1) / titletexwidth - titleborderu, u1 = float(c1+1) / titletexwidth + titleborderu;
		float v0 = float(r0+1) / titletexheight - titleborderv, v1 = float(r1+1) / titletexheight + titleborderv;

		// generate position extents
		float x0 = titlex + c0 * titlew - borderw, x1 = titlex + c1 * titlew + borderw;
		float y0 = titley + r0 * titleh - borderh, y1 = titley + r1 * titleh + borderh;

		// submit vertices
		glTexCoord2f(u0, v0);	glVertex2f(x0, y0);
		glTexCoord2f(u1, v0);	glVertex2f(x1, y0);
		glTexCoord2f(u1, v1);	glVertex2f(x1, y1);
		glTexCoord2f(u0, v1);	glVertex2f(x0, y1);
	}

	glEnd();

	glDisable(GL_TEXTURE_2D);

#endif
}

enum ButtonState
{
	BUTTON_NORMAL = 0,
	BUTTON_SELECTED = 1 << 0,
	BUTTON_ROLLOVER = 1 << 1,
	NUM_BUTTON_STATES = 1 << 2
};

static const Color4 optionbackcolor[NUM_BUTTON_STATES] =
{
	{ 0.2f, 0.2f, 0.2f, 0.5f },
	{ 0.1f, 0.3f, 1.0f, 0.5f },
	{ 0.4f, 0.4f, 0.4f, 0.5f },
	{ 0.1f, 0.7f, 1.0f, 0.5f },
};
static const Color4 optionbordercolor[NUM_BUTTON_STATES] =
{
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
};
static const Color4_2 optionlabelcolor[NUM_BUTTON_STATES] =
{
	{ { 0.1f, 0.6f, 1.0f, 1.0f }, { 0.1f, 0.6f, 1.0f, 1.0f } },
	{ { 1.0f, 0.9f, 0.1f, 1.0f }, { 1.0f, 0.9f, 0.1f, 1.0f } },
	{ { 0.7f, 0.7f, 0.7f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { 1.0f, 0.9f, 0.1f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
};
static const Color4 inertbordercolor[] =
{
	{ 0.1f, 0.1f, 0.1f, 1.0f },
};
static const Color4_2 inertlabelcolor[] =
{
	{ { 0.7f, 0.7f, 0.7f, 1.0f }, { 0.7f, 0.7f, 0.7f, 1.0f } }
};

// shell menu option
struct ShellMenuItem
{
	// option button
	Vector2 mButtonPos;
	Vector2 mButtonSize;
	const Color4 *mButtonColor;

	// option label
	char *mLabel;
	Vector2 mLabelPos;
	Vector2 mLabelJustify;
	Vector2 mCharSize;
	const Color4 *mBorderColor;
	const Color4_2 *mLabelColor;

	// button state
	unsigned int mState;

	// action
	fastdelegate::FastDelegate<void ()> mAction;

	// associated variable
	unsigned int mVariable;
	int mValue;

	// render the button
	void Render(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
	{
		unsigned int state = mState;
		if (VarItem *item = Database::varitem.Get(mVariable))
			if (item->GetInteger() == mValue)
				state |= BUTTON_SELECTED;

		if (mButtonColor)
		{
			// render button
			glBegin(GL_QUADS);
			glColor4fv(mButtonColor[state]);
			glVertex2f(mButtonPos.x, mButtonPos.y);
			glVertex2f(mButtonPos.x + mButtonSize.x, mButtonPos.y);
			glVertex2f(mButtonPos.x + mButtonSize.x, mButtonPos.y + mButtonSize.y);
			glVertex2f(mButtonPos.x, mButtonPos.y + mButtonSize.y);
			glEnd();
		}

		if (mLabel)
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

			glBegin(GL_QUADS);

			// get text corner position
			size_t labellen = strlen(mLabel);
			Vector2 labelcorner(
				mButtonPos.x + mLabelPos.x - mLabelJustify.x * mCharSize.x * labellen,
				mButtonPos.y + mLabelPos.y + (1.0f - mLabelJustify.y) * mCharSize.y);

			if (mBorderColor)
			{
				// render border
				glColor4fv(mBorderColor[state]);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x - 2, labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x    , labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x + 2, labelcorner.y - 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x - 2, labelcorner.y    , mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x + 2, labelcorner.y    , mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x - 2, labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x    , labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
				OGLCONSOLE_DrawString(mLabel, labelcorner.x + 2, labelcorner.y + 2, mCharSize.x, -mCharSize.y, 0);
			}

			// render label
			Color4 color;
			float interp = ((sim_turn & 16) ? 16 - (sim_turn & 15) : (sim_turn & 15)) / 16.0f;
			for (int c = 0; c < 4; c++)
				color[c] = Lerp(mLabelColor[state][0][c], mLabelColor[state][1][c], interp);
			glColor4fv(color);
			OGLCONSOLE_DrawString(mLabel, labelcorner.x, labelcorner.y, mCharSize.x, -mCharSize.y, 0);

			glEnd();

			glDisable(GL_TEXTURE_2D);
		}
	}
};

// shell menu page
struct ShellMenuPage
{
	ShellMenuItem *mOption;
	unsigned int mCount;

	fastdelegate::FastDelegate<void ()> mEnter;
	fastdelegate::FastDelegate<void ()> mExit;

	ShellMenuPage *mParent;
};

// shell menu
struct ShellMenu
{
	ShellMenuPage *mActive;

	void Push(ShellMenuPage *aPage)
	{
		aPage->mParent = mActive;
		if (mActive && mActive->mExit)
			(mActive->mExit)();
		mActive = aPage;
		if (mActive && mActive->mEnter)
			(mActive->mEnter)();
	}
	void Pop()
	{
		if (mActive && mActive->mExit)
			(mActive->mExit)();
		mActive = mActive->mParent;
		if (mActive && mActive->mEnter)
			(mActive->mEnter)();
	}
};

// forward declaration
extern ShellMenu shellmenu;
extern ShellMenuPage shellmenumainpage;
extern ShellMenuPage shellmenuoptionspage;
extern ShellMenuPage shellmenuvideopage;
extern ShellMenuPage shellmenuaudiopage;

//
// MAIN MENU

// start item
void ShellMenuMainPressStart(void)
{
	setgamestate = STATE_PLAY;
}

void ShellMenuMainPressOptions(void)
{
	shellmenu.Push(&shellmenuoptionspage);
}

void ShellMenuMainPressQuit(void)
{
	setgamestate = STATE_QUIT;
}

ShellMenuItem shellmenumainitems[] =
{
	{
		Vector2( 320 - 160, 200 + 80 * 0 ),
		Vector2( 320, 64 ),
		optionbackcolor,
		"START",
		Vector2(160, 32),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressStart,
	},
	{
		Vector2( 320 - 160, 200 + 80 * 1 ),
		Vector2( 320, 64 ),
		optionbackcolor,
		"OPTIONS",
		Vector2(160, 32),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressOptions,
	},
	{
		Vector2( 320 - 160, 200 + 80 * 2 ),
		Vector2( 320, 64 ),
		optionbackcolor,
		"QUIT",
		Vector2(160, 32),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressQuit,
	},
};

//
// OPTION MENU

void ShellMenuOptionsPressVideo()
{
	shellmenu.Push(&shellmenuvideopage);
}

void ShellMenuOptionsPressAudio()
{
	shellmenu.Push(&shellmenuaudiopage);
}

void ShellMenuOptionsPressDebug()
{
}

void ShellMenuOptionsPressBack()
{
	shellmenu.Pop();
}

ShellMenuItem shellmenuoptionsitems[] =
{
	{
		Vector2( 320 - 160, 200 + 64 * 0 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"VIDEO",
		Vector2( 160, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressVideo,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 1 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"AUDIO",
		Vector2( 160, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressAudio,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 2 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"TEST",
		Vector2( 160, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressDebug,
	},
	{
		Vector2( 320 - 100, 200 + 64 * 3 ),
		Vector2( 200, 48 ),
		optionbackcolor,
		"(BACK)",
		Vector2( 100, 24 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuOptionsPressBack,
	},
};


//
// VIDEO MENU

/*
resolution				[-] <width>x<height> [+]
fullscreen				[off] [on]
vertical sync			[off] [on]
multisample				[-] <samples> [+]
motion blur steps		[-] <steps> [+]
motion blur strength	[-] <blur %> [+]
*/

extern ShellMenuItem shellmenuvideoitems[];

SDL_Rect **shellmenuvideoresolutions;
SDL_Rect **shellmenuvideoresolution;
char shellmenuvideoresolutiontext[32];
char shellmenuvideomultisampletext[8];
char shellmenuvideomotionblurstepstext[8];
char shellmenuvideomotionblurtimetext[8];

void ShellMenuVideoEnter()
{
	shellmenuvideoresolutions = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
	shellmenuvideoresolution = shellmenuvideoresolutions;
	for (SDL_Rect **mode = shellmenuvideoresolutions; *mode != NULL; ++mode)
	{
		if ((*mode)->w <= SCREEN_WIDTH && (*mode)->h <= SCREEN_HEIGHT)
			shellmenuvideoresolution = mode;
	}
	TIXML_SNPRINTF(shellmenuvideoresolutiontext, sizeof(shellmenuvideoresolutiontext), "%dx%d", (*shellmenuvideoresolution)->w, (*shellmenuvideoresolution)->h);

	VarItem::CreateInteger("shell.menu.video.fullscreen", SCREEN_FULLSCREEN, 0, 1);
	VarItem::CreateInteger("shell.menu.video.verticalsync", OPENGL_SWAPCONTROL, 0, 1);

	VarItem *varmultisample = VarItem::CreateInteger("shell.menu.video.multisample", OPENGL_MULTISAMPLE, 1, 16);
	TIXML_SNPRINTF(shellmenuvideomultisampletext, sizeof(shellmenuvideomultisampletext), "%dx", varmultisample->GetInteger());

	VarItem *varmotionblursteps = VarItem::CreateInteger("shell.menu.video.motionblur", MOTIONBLUR_STEPS, 1);
	TIXML_SNPRINTF(shellmenuvideomotionblurstepstext, sizeof(shellmenuvideomotionblurstepstext), "%d", varmotionblursteps->GetInteger());

	VarItem *varmotionblurtime = VarItem::CreateInteger("shell.menu.video.motionblurtime", xs_RoundToInt(MOTIONBLUR_TIME * 600), 0, 10);
	TIXML_SNPRINTF(shellmenuvideomotionblurtimetext, sizeof(shellmenuvideomotionblurtimetext), "%d%%", varmotionblurtime->GetInteger() * 10);
}

void ShellMenuVideoExit()
{
	shellmenuvideoresolutions = NULL;
	shellmenuvideoresolution = NULL;
}

void ShellMenuVideoPressAccept()
{
	SCREEN_WIDTH = (*shellmenuvideoresolution)->w;
	SCREEN_HEIGHT = (*shellmenuvideoresolution)->h;
	SCREEN_FULLSCREEN = VarItem::GetInteger("shell.menu.video.fullscreen") != 0;
	OPENGL_SWAPCONTROL = VarItem::GetInteger("shell.menu.video.verticalsync") != 0;
	OPENGL_MULTISAMPLE = VarItem::GetInteger("shell.menu.video.multisample");
	MOTIONBLUR_STEPS = VarItem::GetInteger("shell.menu.video.motionblur");
	MOTIONBLUR_TIME = VarItem::GetInteger("shell.menu.video.motionblurtime") / 600.0f;

	WritePreferences("preferences.xml");
	InitWindowAction();

	shellmenu.Pop();
}

void ShellMenuVideoPressCancel()
{
	shellmenu.Pop();
}

void ShellMenuVideoPressResolutionUp()
{
	if (shellmenuvideoresolution > shellmenuvideoresolutions)
	{
		--shellmenuvideoresolution;
		sprintf(shellmenuvideoresolutiontext, "%dx%d", (*shellmenuvideoresolution)->w, (*shellmenuvideoresolution)->h);
	}
}

void ShellMenuVideoPressResolutionDown()
{
	if (*(shellmenuvideoresolution+1) != NULL)
	{
		++shellmenuvideoresolution;
		sprintf(shellmenuvideoresolutiontext, "%dx%d", (*shellmenuvideoresolution)->w, (*shellmenuvideoresolution)->h);
	}
}

void ShellMenuVideoPressFullScreenOff()
{
	VarItem::SetInteger("shell.menu.video.fullscreen", 0);
}

void ShellMenuVideoPressFullScreenOn()
{
	VarItem::SetInteger("shell.menu.video.fullscreen", 1);
}

void ShellMenuVideoPressVerticalSyncOff()
{
	VarItem::SetInteger("shell.menu.video.verticalsync", 0);
}

void ShellMenuVideoPressVerticalSyncOn()
{
	VarItem::SetInteger("shell.menu.video.verticalsync", 1);
}

void ShellMenuVideoPressMultisampleUp()
{
	if (VarItem *item = Database::varitem.Get(0x31bca13e /* "shell.menu.video.multisample" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuvideomultisampletext, "%dx", item->GetInteger());
	}
}

void ShellMenuVideoPressMultisampleDown()
{
	if (VarItem *item = Database::varitem.Get(0x31bca13e /* "shell.menu.video.multisample" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuvideomultisampletext, "%dx", item->GetInteger());
	}
}

void ShellMenuVideoPressMotionBlurStepsUp()
{
	if (VarItem *item = Database::varitem.Get(0x32e32e54 /* "shell.menu.video.motionblur" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuvideomotionblurstepstext, "%d", item->GetInteger());
	}
}

void ShellMenuVideoPressMotionBlurStepsDown()
{
	if (VarItem *item = Database::varitem.Get(0x32e32e54 /* "shell.menu.video.motionblur" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuvideomotionblurstepstext, "%d", item->GetInteger());
	}
}

void ShellMenuVideoPressMotionBlurTimeUp()
{
	if (VarItem *item = Database::varitem.Get(0xfdcc24f5 /* "shell.menu.video.motionblurtime" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuvideomotionblurtimetext, "%d%%", item->GetInteger() * 10);
	}
}

void ShellMenuVideoPressMotionBlurTimeDown()
{
	if (VarItem *item = Database::varitem.Get(0xfdcc24f5 /* "shell.menu.video.motionblurtime" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuvideomotionblurtimetext, "%d%%", item->GetInteger() * 10);
	}
}

ShellMenuItem shellmenuvideoitems[] =
{
	{
		Vector2( 40, 220 - 24 - 16 ),
		Vector2( 560, 12 ),
		optionbackcolor,
		"VIDEO",
		Vector2( 280, 6 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_SELECTED,
		NULL,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 0 ),
		Vector2( 240, 24 ),
		NULL,
		"Resolution",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressResolutionDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 0 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideoresolutiontext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressResolutionUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 1 ),
		Vector2( 240, 24 ),
		NULL,
		"Full Screen",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 1 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"Off",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressFullScreenOff,
		0x9ddbd712 /* "shell.menu.video.fullscreen" */,
		0
	},
	{
		Vector2( 320 + 20 + 110 + 10, 220 + 32 * 1 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"On",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressFullScreenOn,
		0x9ddbd712 /* "shell.menu.video.fullscreen" */,
		1
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 2 ),
		Vector2( 240, 24 ),
		NULL,
		"Vertical Sync",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 2 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"Off",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressVerticalSyncOff,
		0x97eea3ca /* "shell.menu.video.verticalsync" */,
		0
	},
	{
		Vector2( 320 + 20 + 110 + 10, 220 + 32 * 2 ),
		Vector2( 110, 24 ),
		optionbackcolor,
		"On",
		Vector2( 55, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressVerticalSyncOn,
		0x97eea3ca /* "shell.menu.video.verticalsync" */,
		1
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 3 ),
		Vector2( 240, 24 ),
		NULL,
		"Multisampling",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 3 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMultisampleDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 3 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideomultisampletext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 3 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMultisampleUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 4 ),
		Vector2( 240, 24 ),
		NULL,
		"Motion Steps",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 4 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurStepsDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 4 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideomotionblurstepstext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 4 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurStepsUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 5 ),
		Vector2( 240, 24 ),
		NULL,
		"Motion Blur",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 5 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurTimeDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 5 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuvideomotionblurtimetext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 5 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressMotionBlurTimeUp,
	},
	{
		Vector2( 40, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"ACCEPT",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressAccept,
	},
	{
		Vector2( 600 - 240, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"CANCEL",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuVideoPressCancel,
	},
};

//
// AUDIO MENU

/*
channels				[-] <channels> [+]
volume					[-] <volume %> [+]
test					[-] <sound> [+]
*/

char shellmenuaudiosoundchannelstext[8];
char shellmenuaudiosoundvolumetext[8];

void ShellMenuAudioEnter()
{
	VarItem *varsoundchannels = VarItem::CreateInteger("shell.menu.audio.channels", SOUND_CHANNELS, 1);
	TIXML_SNPRINTF(shellmenuaudiosoundchannelstext, sizeof(shellmenuaudiosoundchannelstext), "%d", varsoundchannels->GetInteger());

	VarItem *varsoundvolume = VarItem::CreateInteger("shell.menu.audio.volume", xs_RoundToInt(SOUND_VOLUME * 10), 0, 20);
	TIXML_SNPRINTF(shellmenuaudiosoundvolumetext, sizeof(shellmenuaudiosoundvolumetext), "%d%%", varsoundvolume->GetInteger() * 10);
}

void ShellMenuAudioExit()
{
}

void ShellMenuAudioPressAccept()
{
	SOUND_CHANNELS = VarItem::GetInteger("shell.menu.audio.channels");
	SOUND_VOLUME = VarItem::GetInteger("shell.menu.audio.volume") / 10.0f;

	WritePreferences("preferences.xml");

	shellmenu.Pop();
}

void ShellMenuAudioPressCancel()
{
	shellmenu.Pop();
}


void ShellMenuAudioPressSoundChannelsUp()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
	}
}

void ShellMenuAudioPressSoundChannelsDown()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
	}
}

void ShellMenuAudioPressSoundVolumeUp()
{
	if (VarItem *item = Database::varitem.Get(0xf97c9992 /* "shell.menu.audio.volume" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundvolumetext, "%d%%", item->GetInteger() * 10);
	}
}

void ShellMenuAudioPressSoundVolumeDown()
{
	if (VarItem *item = Database::varitem.Get(0xf97c9992 /* "shell.menu.audio.volume" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundvolumetext, "%d%%", item->GetInteger() * 10);
	}
}


ShellMenuItem shellmenuaudioitems[] = 
{
	{
		Vector2( 40, 220 - 24 - 16 ),
		Vector2( 560, 12 ),
		optionbackcolor,
		"AUDIO",
		Vector2( 280, 6 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_SELECTED,
		NULL,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 0 ),
		Vector2( 240, 24 ),
		NULL,
		"Mixer Channels",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundChannelsDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 0 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundchannelstext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundChannelsUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 1 ),
		Vector2( 240, 24 ),
		NULL,
		"Mixer Volume",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 1 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 1 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundvolumetext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 1 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeUp,
	},
	{
		Vector2( 40, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"ACCEPT",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressAccept,
	},
	{
		Vector2( 600 - 240, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"CANCEL",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressCancel,
	},
};

//
// TEST MENU

/*
simulation rate			[-] <rate> [+]
time scale				[-] <scale %> [+]
profile screen			[off] [on]
profile print			[off] [on]
framerate screen		[off] [on]
framerate print			[off] [on]
*/

//
// SHELL

ShellMenuPage shellmenumainpage =
{
	shellmenumainitems, SDL_arraysize(shellmenumainitems), NULL, NULL
};
ShellMenuPage shellmenuoptionspage =
{
	shellmenuoptionsitems, SDL_arraysize(shellmenuoptionsitems)
};
ShellMenuPage shellmenuvideopage =
{
	shellmenuvideoitems, SDL_arraysize(shellmenuvideoitems), ShellMenuVideoEnter, ShellMenuVideoExit
};
ShellMenuPage shellmenuaudiopage =
{
	shellmenuaudioitems, SDL_arraysize(shellmenuaudioitems), ShellMenuAudioEnter, ShellMenuAudioExit
};

ShellMenu shellmenu =
{
	0
};

// draw options
void RenderOptions(ShellMenu &menu, unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	// cursor position
	float cursor_x = 320 - 240 * input.value[Input::AIM_HORIZONTAL];
	float cursor_y = 240 - 240 * input.value[Input::AIM_VERTICAL];

	// HACK use the main page
	ShellMenuPage &page = *menu.mActive;

	// for each option on the page...
	for (unsigned int i = 0; i < page.mCount; ++i)
	{
		// get the option
		ShellMenuItem &option = page.mOption[i];

		if (option.mAction)
		{
			// on mouse rollover
			if (cursor_x >= option.mButtonPos.x && cursor_x <= option.mButtonPos.x + option.mButtonSize.x &&
				cursor_y >= option.mButtonPos.y && cursor_y <= option.mButtonPos.y + option.mButtonSize.y)
			{
				// mark as rollover
				option.mState |= BUTTON_ROLLOVER;

				// if mouse button pressed...
				if (input.value[Input::FIRE_PRIMARY])
				{
					// mark as selected
					option.mState |= BUTTON_SELECTED;
				}
				else if (option.mState & BUTTON_SELECTED)
				{
					// mark as not selected
					option.mState &= ~BUTTON_SELECTED;

					// perform action
					(option.mAction)();
				}
			}
			else
			{
				// mark as not rollover
				option.mState &= ~BUTTON_ROLLOVER;

				if (!input.value[Input::FIRE_PRIMARY])
				{
					// mark as not selected
					option.mState &= ~BUTTON_SELECTED;
				}
			}
		}

		// render the option
		option.Render(aId, aTime, aPosX, aPosY, aAngle);
	}

	// draw reticule (HACK)
	glPushMatrix();
	glTranslatef(cursor_x, cursor_y, 0.0f);
	glCallList(reticule_handle);
	glPopMatrix();
}

// render shell options
void RenderShellOptions(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	RenderOptions(shellmenu, aId, aTime, aPosX, aPosY, aAngle);
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

	// input binding
	InitInput(INPUT_CONFIG.c_str());

	// level configuration
	InitLevel("shell.xml");

	// start audio
	SDL_PauseAudio(0);

	// create title overlay
	Overlay *title = new Overlay(0x9865b509 /* "title" */);
	Database::overlay.Put(0x9865b509 /* "title" */, title);
	title->SetAction(Overlay::Action(RenderShellTitle));
	title->Show();

	// create options overlay
	shellmenu.mActive = NULL;
	shellmenu.Push(&shellmenumainpage);
	Overlay *options = new Overlay(0xef286ca5 /* "options" */);
	Database::overlay.Put(0xef286ca5 /* "options" */, options);
	options->SetAction(Overlay::Action(RenderShellOptions));
	options->Show();

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

//
// ESCAPE MENU

extern void EscapeMenuExit();

void EscapeMainMenuPressContinue(void)
{
	EscapeMenuExit();
}

void EscapeMainMenuPressRestart(void)
{
	setgamestate = STATE_RELOAD;
	EscapeMenuExit();
}

void EscapeMainMenuPressMain(void)
{
	setgamestate = STATE_SHELL;
	EscapeMenuExit();
}

ShellMenuItem escapemenumainitems[] =
{
	{
		Vector2( 320 - 160, 200 + 64 * 0 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"CONTINUE",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		EscapeMainMenuPressContinue,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 1 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"RESTART",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		EscapeMainMenuPressRestart,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 2 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"OPTIONS",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuMainPressOptions,
	},
	{
		Vector2( 320 - 160, 200 + 64 * 3 ),
		Vector2( 320, 48 ),
		optionbackcolor,
		"MAIN",
		Vector2(160, 24),
		Vector2(0.5f, 0.5f),
		Vector2(32, 24),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		EscapeMainMenuPressMain,
	},
};

ShellMenuPage escapemenumainpage =
{
	escapemenumainitems, SDL_arraysize(escapemenumainitems), NULL, NULL
};

// render shell options
void RenderEscapeOptions(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	// darken the screen
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(640, 0);
	glVertex2f(640, 480);
	glVertex2f(0, 480);
	glEnd();

	// render options
	RenderOptions(shellmenu, aId, aTime, aPosX, aPosY, aAngle);
}


// drain values
static const float DRAIN_DELAY = 1.0f;
static const float DRAIN_RATE = 0.5f;

// flash values
static const int MAX_FLASH = 16;
static const float FLASH_RATE = 2.0f;

template<typename T> struct Rect
{
	T x;
	T y;
	T w;
	T h;
};

// health gauge
static const Rect<float> healthrect =
{
	8, 8, 128, 8
};
static const Color4 healthcolor[3][2] =
{
	{ { 1.0f, 0.0f, 0.0f, 1.0f },  { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { 1.0f, 1.0f, 0.0f, 0.75f }, { 1.0f, 1.0f, 0.3f, 0.75f } },
	{ { 0.0f, 1.0f, 0.0f, 0.5f },  { 0.2f, 1.0f, 0.2f, 0.5f } },
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
	unsigned int trackid;
	Vector2 trackpos[2];
	Vector2 trackaim;

	// reticule values
	Vector2 aimpos[2];

public:
	PlayerHUD(unsigned int aPlayerId);
	~PlayerHUD();

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
	trackid = 0;
	trackpos[0] = Vector2(0, 0);
	trackpos[1] = Vector2(0, 0);
	trackaim = Vector2(0, 0);
	aimpos[0] = Vector2(0, 0);
	aimpos[1] = Vector2(0, 0);

	Updatable::SetAction(Updatable::Action(this, &PlayerHUD::Update));
	Overlay::SetAction(Overlay::Action(this, &PlayerHUD::Render));
}

PlayerHUD::~PlayerHUD()
{
}

void PlayerHUD::Update(float aStep)
{
	// get the player
	Player *player = Database::player.Get(Updatable::mId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;
	if (id)
	{
		// get the entity
		if (Entity *entity = Database::entity.Get(id))
		{
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
		}
	}

	// if the tracked identifier changes...
	if (trackid != id)
	{
		// snap position
		trackid = id;
		trackpos[0] = trackpos[1];
	}

	// set camera position
	camerapos[0] = trackpos[0];
	camerapos[1] = trackpos[1];

#ifdef TEST_PATHING
	Pathing(entity->GetPosition(), trackpos[1] + aimpos[1] * 120 * VIEW_SIZE / 320, 4.0f);
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

	Color4 fillcolor;
	for (int i = 0; i < 4; i++)
		fillcolor[i] = Lerp(Lerp(healthcolor[band][0][i], healthcolor[band+1][0][i], ratio), Lerp(healthcolor[band][1][i], healthcolor[band+1][1][i], ratio), pulse);

	// begin drawing
	glBegin(GL_QUADS);

	// background
	glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
	glVertex2f(healthrect.x, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w, healthrect.y + healthrect.h);
	glVertex2f(healthrect.x, healthrect.y + healthrect.h);

	// drain
	glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * drain, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * drain, healthrect.y + healthrect.h);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h);

	// flash
	for (int i = 0; i < flashcount; ++i)
	{
		Flash &flashinfo = flash[i];
		glColor4f(1.0f, 1.0f, 1.0f, flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.left, healthrect.y - 2 * flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.right, healthrect.y - 2 * flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.right, healthrect.y + healthrect.h + 2 * flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.left, healthrect.y + healthrect.h + 2 * flashinfo.fade);
	}

	// fill gauge
	glColor4fv(fillcolor);
	glVertex2f(healthrect.x, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h);
	glVertex2f(healthrect.x, healthrect.y + healthrect.h);

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

			// draw the player ship
			glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
			glPushMatrix();
			glTranslatef(healthrect.x + healthrect.w + 8, healthrect.y + 4, 0.0f);
			glScalef(-0.5, -0.5, 1);
			RenderDrawlist(0xeec1dafa /* "playership" */, 0.0f, 0.0f, 0.0f, 0.0f);
			glPopMatrix();

			// draw remaining lives
			char lives[16];
			sprintf(lives, "x%d", cur_lives);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

			glBegin(GL_QUADS);
			float w = 8;
			float h = -8;
			float x = healthrect.x + healthrect.w + 16;
			float y = healthrect.y + 4 - 0.5f * h;
			float z = 0;
			OGLCONSOLE_DrawString(lives, x, y, w, h, z);

			glEnd();

			glDisable(GL_TEXTURE_2D);

			glEndList();
		}
	}

	// get "special" ammo resource (HACK)

	int cur_ammo = -1;
	int new_ammo = INT_MAX;
	if (Resource *specialammo = Database::resource.Get(aId).Get(0xd940d530 /* "special" */))
	{
		new_ammo = xs_FloorToInt(specialammo->GetValue());
	}
	if (new_ammo < INT_MAX)
	{
		// if the ammo has not changed...
		if (new_ammo == cur_ammo && !wasreset)
		{
			// call the existing draw list
			glCallList(ammo_handle);
		}
		else
		{
			// update ammo
			cur_ammo = new_ammo;

			// start a new draw list list
			glNewList(ammo_handle, GL_COMPILE_AND_EXECUTE);

			// draw the special ammo
			glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
			glPushMatrix();
			glTranslatef(healthrect.x + healthrect.w + 8, healthrect.y + 16, 0.0f);
			glScalef(4, 4, 1);
			glCallList(Database::drawlist.Get(0x8cdedbba /* "circle16" */));
			glPopMatrix();

			// draw remaining ammo
			char ammo[16];
			sprintf(ammo, "x%d", cur_ammo);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

			glBegin(GL_QUADS);
			float w = 8;
			float h = -8;
			float x = healthrect.x + healthrect.w + 16;
			float y = healthrect.y + 16 - 0.5f * h;
			float z = 0;
			OGLCONSOLE_DrawString(ammo, x, y, w, h, z);

			glEnd();

			glDisable(GL_TEXTURE_2D);

			glEndList();
		}
	}

	static float gameovertimer = 0.0f;

	// if tracking an active controller...
	Controller *controller = Database::controller.Get(id);
	if (controller)
	{
		// draw reticule
		float x = 320 - 240 * Lerp(aimpos[0].x, aimpos[1].x, sim_fraction);
		float y = 240 - 240 * Lerp(aimpos[0].y, aimpos[1].y, sim_fraction);

		glPushMatrix();
		glTranslatef(x, y, 0.0f);
		glCallList(reticule_handle);
		glPopMatrix();

		gameovertimer = 0.0f;
	}
	else if (cur_lives <= 0)
	{
		// display game over
		gameovertimer += frame_time;
		if (gameovertimer > 1.0f)
			gameovertimer = 1.0f;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);


		glBegin(GL_QUADS);

		static char *text = "GAME OVER";
		const float w = Lerp<float>(96, 48, gameovertimer);
		const float h = Lerp<float>(-64, -32, gameovertimer);
		const float x = 320 - 0.5f * w * strlen(text);
		const float y = 240 - 0.5f * h;
		const float z = 0;
		const float a = gameovertimer * gameovertimer;

		glColor4f(0.1f, 0.1f, 0.1f, a);
		OGLCONSOLE_DrawString(text, x - 2, y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x    , y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x - 2, y    , w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y    , w, h, z);
		OGLCONSOLE_DrawString(text, x - 2, y + 2, w, h, z);
		OGLCONSOLE_DrawString(text, x    , y + 2, w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y + 2, w, h, z);

		glColor4f(1.0f, 0.2f, 0.1f, a);
		OGLCONSOLE_DrawString(text, x, y, w, h, z);

		glEnd();

		glDisable(GL_TEXTURE_2D);
	}
}

namespace Database
{
	Typed<PlayerHUD *> playerhud(0x8e522e29 /* "playerhud" */);
}

// player join
void PlayerJoinListener(unsigned int aId)
{
	// create player hud overlay
	PlayerHUD *playerhud = new PlayerHUD(aId);
	Database::playerhud.Put(aId, playerhud);
	playerhud->Activate();
	playerhud->Show();
}

// player quit
void PlayerQuitListener(unsigned int aId)
{
	if (PlayerHUD *playerhud = Database::playerhud.Get(aId))
	{
		// remove player hud overlay
		delete playerhud;
		Database::playerhud.Delete(aId);
	}
}

// enter escape menu
void EscapeMenuEnter()
{
	escape = true;
	if (Overlay *overlay = Database::overlay.Get(0x9e212406 /* "escape" */))
	{
		for (Database::Typed<PlayerHUD *>::Iterator itor(&Database::playerhud); itor.IsValid(); ++itor)
			itor.GetValue()->Hide();
		shellmenu.mActive = NULL;
		shellmenu.Push(&escapemenumainpage);
		overlay->Show();
	}
}

// exit escape menu
void EscapeMenuExit()
{
	if (Overlay *overlay = Database::overlay.Get(0x9e212406 /* "escape" */))
	{
		for (Database::Typed<PlayerHUD *>::Iterator itor(&Database::playerhud); itor.IsValid(); ++itor)
			itor.GetValue()->Show();
		overlay->Hide();
	}
	escape = false;
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

	// reset camera position
	camerapos[0] = camerapos[1] = Vector2(0, 0);

	// reset simulation timer
	sim_rate = float(SIMULATION_RATE);
	sim_step = 1.0f / sim_rate;
	sim_turn = 0;
	sim_fraction = 1.0f;

	// input binding
	InitInput(INPUT_CONFIG.c_str());

	// level configuration
	if (!InitLevel(LEVEL_CONFIG.c_str()))
		setgamestate = STATE_SHELL;

	// start audio
	SDL_PauseAudio(0);

	// add a join listener
	Database::playerjoin.Put(0xe28d61c6 /* "hud" */, PlayerJoinListener);

	// add a quit listener
	Database::playerquit.Put(0xe28d61c6 /* "hud" */, PlayerQuitListener);

	// create escape overlay
	Overlay *escape = new Overlay(0x9e212406 /* "escape" */);
	Database::overlay.Put(0x9e212406 /* "escape" */, escape);
	escape->SetAction(Overlay::Action(RenderEscapeOptions));

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

	// delete escape overlay
	delete Database::overlay.Get(0x9e212406 /* "escape" */);
	Database::overlay.Delete(0x9e212406 /* "escape" */);

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
	LONGLONG overlay_time[NUM_SAMPLES] = { 0 };
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
		overlay_time[profile_index] = 0;
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
				switch (event.key.keysym.sym)
				{
				case SDLK_F4:
					if (event.key.keysym.mod & KMOD_ALT)
						setgamestate = STATE_QUIT;
					break;
				case SDLK_RETURN:
					if (event.key.keysym.mod & KMOD_ALT)
					{
						SCREEN_FULLSCREEN = !SCREEN_FULLSCREEN;
						init_Window();
					}
					break;
				case SDLK_ESCAPE:
					if (curgamestate == STATE_PLAY)
					{
						if (escape)
							EscapeMenuExit();
						else
							EscapeMenuEnter();
					}
					break;
				case SDLK_PAUSE:
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
					break;
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
			// advance 1/60th of a second
			frame_time = TIME_SCALE / 60.0f;
			frame_turns = frame_time * sim_rate;
		}
		else if (paused || escape)
		{
			// freeze time
			frame_time = 0.0f;
			frame_turns = 0.0f;
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
		float step_turns = std::min(TIME_SCALE * MOTIONBLUR_TIME * sim_rate, 1.0f) / MOTIONBLUR_STEPS;

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

			if (escape)
			{
				input.Update();
				input.Step();
			}
			// while simulation turns to run...
			else while ((singlestep || !paused) && sim_fraction >= 1.0f)
			{
				// deduct a turn
				sim_fraction -= 1.0f;
				
				// advance the turn counter
				++sim_turn;

				// save original fraction
				float save_fraction = sim_fraction;

				// switch fraction to simulation mode
				sim_fraction = 0.0f;

#ifdef COLLECT_DEBUG_DRAW
				// collect any debug draw
				glNewList(debugdraw, GL_COMPILE);
#endif

				// seed the random number generator
				randlongseed = sim_turn;
				(void)RandLong();

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
						inputlognext->QueryIntAttribute("fire3", reinterpret_cast<int *>(&input.output[Input::FIRE_CHANNEL3]));
						inputlognext->QueryIntAttribute("fire4", reinterpret_cast<int *>(&input.output[Input::FIRE_CHANNEL4]));

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
						if (input.output[Input::FIRE_CHANNEL3] != prev[Input::FIRE_CHANNEL3])
							item.SetAttribute( "fire3", *reinterpret_cast<int *>(&input.output[Input::FIRE_CHANNEL3]));
						if (input.output[Input::FIRE_CHANNEL4] != prev[Input::FIRE_CHANNEL4])
							item.SetAttribute( "fire4", *reinterpret_cast<int *>(&input.output[Input::FIRE_CHANNEL4]));

						// add the new input entry
						inputlogroot->InsertEndChild(item);
					}
				}
				else
				{
					// update input values
					input.Update();
				}

				// do any pending turn actions
				DoTurn();


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

#ifdef COLLECT_DEBUG_DRAW
				// finish the draw list
				glEndList();
#endif
				
				// restore original fraction
				sim_fraction = save_fraction;
			}

			// clear single-step
			singlestep = false;

			// seed the random number generator
			randlongseed = sim_turn ^ *(unsigned long *)&sim_fraction;
			(void)RandLong();

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

			// get interpolated track position
			Vector2 viewpos(Lerp(camerapos[0], camerapos[1], sim_fraction));

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

#ifdef GET_PERFORMANCE_DETAILS
		LARGE_INTEGER perf_count1;
		QueryPerformanceCounter(&perf_count1);
		render_time[profile_index] += perf_count1.QuadPart - perf_count0.QuadPart;
#endif

#ifdef COLLECT_DEBUG_DRAW
		// push camera transform
		glPushMatrix();

		// get interpolated track position
		Vector2 viewpos(Lerp(camerapos[0], camerapos[1], sim_fraction));

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
		LARGE_INTEGER perf_count2;
		QueryPerformanceCounter(&perf_count2);
		overlay_time[profile_index] += perf_count2.QuadPart - perf_count1.QuadPart;

		if (!OPENGL_SWAPCONTROL)
		{
			// wait for rendering to finish
			glFinish();

			LARGE_INTEGER perf_count3;
			QueryPerformanceCounter(&perf_count3);
			display_time[profile_index] += perf_count3.QuadPart - perf_count2.QuadPart;
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
			static BandInfo band_info[] =
			{
				{ control_time,		1.0f,	0.0f,	0.0f,	0.5f },
				{ simulate_time,	1.0f,	1.0f,	0.0f,	0.5f },
				{ collide_time,		0.0f,	1.0f,	0.0f,	0.5f },
				{ update_time,		0.0f,	0.5f,	1.0f,	0.5f },
				{ render_time,		1.0f,	0.0f,	1.0f,	0.5f },
				{ overlay_time,		1.0f,	0.5f,	0.0f,	0.5f },
				{ display_time,		0.5f,	0.5f,	0.5f,	0.5f },
			};

			// generate y samples
			float sample_y[SDL_arraysize(band_info)+1][NUM_SAMPLES];
			int index = profile_index;
			for (int i = 0; i < NUM_SAMPLES; ++i)
			{
				float y = 480.0f;
				sample_y[0][i] = y;
				for (int band = 0; band < SDL_arraysize(band_info); ++band)
				{
					y -= 60.0f * 480.0f * band_info[band].time[index] / perf_freq.QuadPart;
					sample_y[band+1][i] = y;
				}
				if (++index >= NUM_SAMPLES)
					index = 0;
			}

			glBegin(GL_QUADS);
			for (int band = 0; band < SDL_arraysize(band_info); ++band)
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
			DebugPrint("C=%d S=%d P=%d U=%d R=%d O=%d D=%d\n",
				1000000 * control_time[profile_index] / perf_freq.QuadPart,
				1000000 * simulate_time[profile_index] / perf_freq.QuadPart,
				1000000 * collide_time[profile_index] / perf_freq.QuadPart,
				1000000 * update_time[profile_index] / perf_freq.QuadPart,
				1000000 * render_time[profile_index] / perf_freq.QuadPart,
				1000000 * overlay_time[profile_index] / perf_freq.QuadPart,
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

		// show the screen
		SDL_GL_SwapBuffers();

#ifdef GET_PERFORMANCE_DETAILS
		if (OPENGL_SWAPCONTROL)
#endif
		// wait for rendering to finish
		glFinish();

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


//
// RELOAD STATE

void ReloadState()
{
	setgamestate = STATE_PLAY;
}