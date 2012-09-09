// precompiled header
#include "stdafx.h"

// includes
#include "Console.h"
#include "GameState.h"
#include "Preferences.h"
#include "Sound.h"
#include "Command.h"
#include "Render.h"
#include "Drawlist.h"
#include "Texture.h"

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
float VIEW_SIZE = 240;

// opengl attributes
bool OPENGL_SWAPCONTROL = true;
int OPENGL_MULTISAMPLE = 4;

// debug output
bool DEBUGPRINT_OUTPUTCONSOLE = false;
#ifdef _DEBUG
bool DEBUGPRINT_OUTPUTDEBUG = true;
#else
bool DEBUGPRINT_OUTPUTDEBUG = false;
#endif
bool DEBUGPRINT_OUTPUTSTDERR = false;

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

// console
Console *console;


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
		console->Print("%s", buf);
	if (DEBUGPRINT_OUTPUTSTDERR)
		fputs(buf, stderr);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

void cmdCB(const char *cmd)
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


bool InitOpenGL()
{	
	// set clear color
	glClearColor( 0, 0, 0, 0 );

	// set viewport
	glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );

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
	// disable z test
	glDisable( GL_DEPTH_TEST );
#endif

	// diable fog by default
	glDisable( GL_FOG );

	// allow vertex arrays
	glEnableClientState(GL_VERTEX_ARRAY);

	// return true if no errors
	return glGetError() == GL_NO_ERROR;
}


void PrintAttributes(void)
{
	DebugPrint("\nOpenGL\n");

	// OpenGL attributes
	DebugPrint( "Vendor     : %s\n", glGetString( GL_VENDOR ) );
	DebugPrint( "Renderer   : %s\n", glGetString( GL_RENDERER ) );
	DebugPrint( "Version    : %s\n", glGetString( GL_VERSION ) );

	// OpenGL extensions
	DebugPrint( "Extensions : \n" );
	const GLubyte *extensions = glGetString( GL_EXTENSIONS );
	size_t size = strlen(reinterpret_cast<const char *>(extensions))+1;
	char *buf = static_cast<char *>(_alloca(size));
	memcpy(buf, extensions, size);

	char *extension = strtok(buf, " ");
	do
	{
		DebugPrint( "%s\n", extension );
		extension = strtok(NULL, " ");
	}
	while(extension);
	DebugPrint("\n");
}

bool OpenWindow(void)
{
	if (runtime)
	{
		// platform-specific open
		if (!Platform::OpenWindow())
			return false;
	}

	// initialize OpenGL
	if( !InitOpenGL() )
		return false;    

	if (runtime)
	{
		// rebuild rendering
		PostResetRender();

		// rebuild textures
		RebuildTextures();

		// post-reset draw lists
		PostResetDrawlists();

		// rebuild console
		console->Resize();
	}

	return true;
}

void CloseWindow(void)
{
	if (runtime)
	{
		// pre-reset draw lists
		PreResetDrawlists();

		// pre-reset rendering
		PreResetRender();

		// platform-specific close
		Platform::CloseWindow();
	}
}

bool Init(void)
{
	// platform-specific initialization
	Platform::Init();

	// create window
	OpenWindow();

	// hide the mouse cursor
	Platform::ShowCursor(false);

	// grab input
	Platform::GrabInput(true);

	// print OpenGL attributes
	PrintAttributes();

	// initialize console
	console = new Console(cmdCB);

	// initialize sound system
	Sound::Init();

	// pause audio
	Sound::Pause();

	// success!
	return true;    
}

void Done(void)
{
    // clean up console
	delete console;

	// clean up sound system
	Sound::Done();

	// close window
	CloseWindow();

	// platform-specific cleanup
	Platform::Done();
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
		_controlfp_s(&prev, 0, /*_EM_ZERODIVIDE|*/_EM_INVALID);

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
	if( !Init() )
		return 1;    

	// run game state machine
	while (GameStateUpdate());

	// clean up
	Done();

	// done
	return 0;
}
