// precompiled header
#include "stdafx.h"

// includes
#include "oglconsole.h"
#include "GameState.h"
#include "Shell.h"
#include "World.h"
#include "Preferences.h"
#include "Sound.h"
#include "Variable.h"
#include "Command.h"
#include "Drawlist.h"

#include <cstdarg>

#ifdef USE_PATHING
#include "Pathing.h"
#endif

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

// default input configuration
std::string INPUT_CONFIG = "input/default.xml";

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

#define TRACE_OPENGL_ATTRIBUTES

// forward declaration
int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );

#if defined(USE_GLFW)
// input callbacks
extern void KeyCallback(int aIndex, int aState);
extern void MousePosCallback(int aPosX, int aPosY);
extern void MouseButtonCallback(int aIndex, int aState);
extern void MouseWheelCallback(int aPos);
#endif


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

	// disable lighting
	glDisable( GL_LIGHTING );

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

	glDisable( GL_FOG );
	/*
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
	*/

	// return true if no errors
	return glGetError() == GL_NO_ERROR;
}

bool init_Window()
{
#if defined(USE_SDL)
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
#elif defined(USE_SFML)
	// create the window
	window.Create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32), "Shmup!", SCREEN_FULLSCREEN ? sf::Style::Fullscreen : sf::Style::Close, sf::WindowSettings(32, 0, OPENGL_MULTISAMPLE));
	window.UseVerticalSync(OPENGL_SWAPCONTROL);
#elif defined(USE_GLFW)
	glfwOpenWindowHint(GLFW_ACCUM_RED_BITS, 16);
	glfwOpenWindowHint(GLFW_ACCUM_GREEN_BITS, 16);
	glfwOpenWindowHint(GLFW_ACCUM_BLUE_BITS, 16);
	glfwOpenWindowHint(GLFW_ACCUM_ALPHA_BITS, 16);
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, OPENGL_MULTISAMPLE);
	glfwOpenWindow(SCREEN_WIDTH, SCREEN_HEIGHT, 8, 8, 8, 8, 0, 0, SCREEN_FULLSCREEN ? GLFW_FULLSCREEN : GLFW_WINDOW);
	glfwSetWindowTitle("Shmup!");
	glfwSwapInterval( OPENGL_SWAPCONTROL );
#else
#error
#endif

	// device was reset
	wasreset = true;

	// initialize OpenGL
	if( !init_GL() )
		return false;    

	if (runtime)
	{
		// rebuild textures
		RebuildTextures();

		// rebuild draw lists
		RebuildDrawlists();

		// rebuild console
		OGLCONSOLE_CreateFont();
		OGLCONSOLE_Resize(console);
	}

	return true;
}

bool init()
{
#if defined(USE_SDL)
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
#elif defined (USE_GLFW)
	// initialize GLFW
	glfwInit();
#endif

	// initialize the window
	init_Window();

#if defined(USE_SDL)
	// hide the mouse cursor
	SDL_ShowCursor(SDL_DISABLE);

	// grab the cursor
	SDL_WM_GrabInput(SDL_GRAB_ON);

	// set window title
	SDL_WM_SetCaption( "Shmup!", NULL );
#elif defined(USE_SFML)
	// hide the mouse cursor
	window.ShowMouseCursor(false);
#elif defined(USE_GLFW)
	// hide the mouse cursor
	glfwDisable(GLFW_MOUSE_CURSOR);

	// set callbacks
	glfwSetKeyCallback(KeyCallback);
	glfwSetMousePosCallback(MousePosCallback);
	glfwSetMouseButtonCallback(MouseButtonCallback);
	glfwSetMouseWheelCallback(MouseWheelCallback);
#endif

    /* Initialize OGLCONSOLE */                                                                      
    console = OGLCONSOLE_Create();                                                                             
	OGLCONSOLE_EditConsole(console);
    OGLCONSOLE_EnterKey(cmdCB);                                                                      

#ifdef TRACE_OPENGL_ATTRIBUTES
#if defined(USE_SDL)
	DebugPrint("Screen BPP: %d\n", SDL_GetVideoSurface()->format->BitsPerPixel);
	DebugPrint("\n");
#endif
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

#if defined(USE_SDL)
	int value;
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
#endif

	// initialize sound system
	Sound::Init();

	// success!
	return true;    
}

void clean_up()
{
    /* clean up oglconsole */                                                                        
    OGLCONSOLE_Quit();

	// clean up sound system
	Sound::Done();

#if defined(USE_SDL)

	// quit SDL
	SDL_Quit();

#elif defined(USE_GLFW)

	// terminate GLFW
	glfwTerminate();

#endif
}


// main
#if defined(USE_SDL)
int SDL_main( int argc, char *argv[] )
#elif defined(WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main( int argc, char *argv[] )
#endif
{
#ifdef WIN32
	if (IsDebuggerPresent())
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

#if !defined(USE_SDL) && defined(WIN32)
	int argc = 1;
	char *argv[64] = { NULL };
	int argsize = strlen(lpCmdLine) + 1;
	char *argdata = static_cast<char *>(_alloca(argsize));
	memcpy(argdata, lpCmdLine, argsize);
	{
		for (char *ptr = strtok(argdata, " \t"); ptr != NULL; ptr = strtok(NULL, " \t"))
		{
			argv[argc++] = ptr;
		}
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

	// run game state machine
	while (GameStateUpdate());

	// clean up
	clean_up();

	// done
	return 0;
}
