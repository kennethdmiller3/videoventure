// precompiled header
#include "stdafx.h"

// includes
#include "oglconsole.h"
#include "Shell.h"
#include "World.h"
#include "Player.h"
#include "PlayerCamera.h"
#include "PlayerHUD.h"
#include "Preferences.h"
#include "Sound.h"
#include "Variable.h"
#include "Command.h"

#include "Collidable.h"
#include "Controller.h"
#include "Simulatable.h"
#include "Renderable.h"
#include "Drawlist.h"
#include "Overlay.h"

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

// sound attributes
int SOUND_CHANNELS = 8;
float SOUND_VOLUME = 1.0f;

// default input configuration
std::string INPUT_CONFIG = "input/default.xml";

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
unsigned int Random::gSeed = 0x92D68CA2;

// camera position (HACK)
Vector2 camerapos[2];

// reticule handle (HACK)
GLuint reticule_handle;

// forward declaration
int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );
void RunState();
void EnterShellState();
void ExitShellState();
void EnterPlayState();
void ExitPlayState();
void ReloadState();

// game state machine (HACK)
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

#if defined(USE_GLFW)

// forward declaration
bool init_Window();
void EscapeMenuExit();
void EscapeMenuEnter();

void KeyCallback(int aIndex, int aState)
{
	if (aState == GLFW_PRESS)
	{
		input.OnPress(Input::TYPE_KEYBOARD, 0, aIndex);

		switch (aIndex)
		{
		case GLFW_KEY_F4:
			if (glfwGetKey(GLFW_KEY_LALT) || glfwGetKey(GLFW_KEY_RALT))
				setgamestate = STATE_QUIT;
			break;
		case GLFW_KEY_ENTER:
			if (glfwGetKey(GLFW_KEY_LALT) || glfwGetKey(GLFW_KEY_RALT))
			{
				SCREEN_FULLSCREEN = !SCREEN_FULLSCREEN;
				init_Window();
			}
			break;
		case GLFW_KEY_ESC:
			if (curgamestate == STATE_PLAY)
			{
				if (escape)
					EscapeMenuExit();
				else
					EscapeMenuEnter();
			}
			break;
		case 'P':
			if (glfwGetKey(GLFW_KEY_LSHIFT) || glfwGetKey(GLFW_KEY_RSHIFT))
			{
				paused = true;
				singlestep = true;
			}
			else
			{
				paused = !paused;
			}
			if (paused)
				glfwEnable(GLFW_MOUSE_CURSOR);
			else
				glfwDisable(GLFW_MOUSE_CURSOR);
			break;
		}
	}
	else
	{
		input.OnRelease(Input::TYPE_KEYBOARD, 0, aIndex);
	}
}

void MousePosCallback(int aPosX, int aPosY)
{
	input.OnAxis(Input::TYPE_MOUSE_AXIS, 0, 0, float(aPosX * 2 - SCREEN_WIDTH) / float(SCREEN_HEIGHT));
	input.OnAxis(Input::TYPE_MOUSE_AXIS, 0, 1, float(aPosY * 2 - SCREEN_HEIGHT) / float(SCREEN_HEIGHT));
}

void MouseButtonCallback(int aIndex, int aState)
{
	if (aState == GLFW_PRESS)
		input.OnPress(Input::TYPE_MOUSE_BUTTON, 0, aIndex);
	else
		input.OnRelease(Input::TYPE_MOUSE_BUTTON, 0, aIndex);
}

void MouseWheelCallback(int aPos)
{
}

#endif

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

#if defined(USE_SDL)
	SDL_AudioSpec fmt;
	fmt.freq = AUDIO_FREQUENCY;
	fmt.format = AUDIO_S16SYS;
	fmt.channels = 2;
	fmt.samples = Uint16(AUDIO_FREQUENCY / SIMULATION_RATE);
	fmt.callback = Sound::Mix;
	fmt.userdata = &Sound::listenerpos;

	/* Open the audio device and start playing sound! */
	if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
		DebugPrint("Unable to open audio: %s\n", SDL_GetError());
	}
#endif

	// success!
	return true;    
}

void clean_up()
{
    /* clean up oglconsole */                                                                        
    OGLCONSOLE_Quit();

#if defined(USE_SDL)

	SDL_CloseAudio();

	// quit SDL
	SDL_Quit();

#elif defined(USE_GLFW)

	// terminate GLFW
	glfwTerminate();

#endif
}


bool InitInput(const char *config)
{
	// clear existing bindings
	input.Clear();

	// load input binding file
	DebugPrint("Input %s\n", config);
	TiXmlDocument document(config);
	if (!document.LoadFile())
		DebugPrint("error loading input file \"%s\": %s\n", config, document.ErrorDesc());

	// process child elements of the root
	if (const TiXmlElement *root = document.FirstChildElement("input"))
	{
		input.Configure(root);
		return true;
	}

	return false;
}

bool SplitLevel(const char *config, const char *output)
{
	// load level data file
	DebugPrint("Level %s -> %s\n", config, output);
	TiXmlDocument document(config);
	document.SetCondenseWhiteSpace(false);
	if (!document.LoadFile())
		DebugPrint("error loading level file \"%s\": %s\n", config, document.ErrorDesc());

	// if the document has a world section...
	if (const TiXmlElement *root = document.FirstChildElement("world"))
	{
		// output level data file
		TiXmlDocument split(output);
		split.SetCondenseWhiteSpace(false);
		TiXmlElement *splitroot = NULL;

		// copy nodes from the original
		for (const TiXmlNode *node = document.FirstChild(); node != NULL; node = node->NextSibling())
		{
			if (node == root)
			{
				splitroot = new TiXmlElement(root->Value());
				for (const TiXmlAttribute *attribute = root->FirstAttribute(); attribute != NULL; attribute = attribute->Next())
					splitroot->SetAttribute(attribute->Name(), attribute->Value());
				split.LinkEndChild(splitroot);
			}
			else
			{
				split.InsertEndChild(*node);
			}
		}

		// for each node...
		for (const TiXmlNode *node = root->FirstChild(); node != NULL; node = node->NextSibling())
		{
			// if the node is an element...
			if (const TiXmlElement *element = node->ToElement())
			{
				// if the element is not an instance
				if (Hash(element->Value()) != 0xd33ff5da /* "entity" */)
				{
					// if the element has a name...
					if (const char *name = element->Attribute("name"))
					{
						// export child element as a separate XML file
						char filename[256];
						TIXML_SNPRINTF(filename, sizeof(filename), "%s/%s.xml", element->Value(), name);
						DebugPrint("%s\n", filename);
						TiXmlDocument piece(filename);
						TiXmlDeclaration * declaration = new TiXmlDeclaration( "1.0", "", "" );
						piece.LinkEndChild(declaration);
						piece.InsertEndChild(*element);
						if (piece.SaveFile())
						{
							// insert an import element
							TiXmlElement *import = new TiXmlElement("import");
							import->SetAttribute("name", filename);
							splitroot->LinkEndChild(import);

							continue;
						}
					}
				}
			}

			// copy to the output
			splitroot->InsertEndChild(*node);
		}

		split.SaveFile();

		return true;
	}

	return false;
}

bool InitLevel(const char *config)
{
	// load level data file
	DebugPrint("Level %s\n", config);
	TiXmlDocument document(config);
	if (!document.LoadFile())
		DebugPrint("error loading level file \"%s\": %s\n", config, document.ErrorDesc());

	// if the document has a world section...
	if (const TiXmlElement *root = document.FirstChildElement("world"))
	{
		// process the world
		ProcessWorldItem(root);

		// get the reticule draw list (HACK)
		reticule_handle = Database::drawlist.Get(0x170e4c58 /* "reticule" */);

		// play the startup sound (HACK)
		PlaySoundCue(0x94326baa /* "startup" */);

		return true;
	}

	// clear the reticule draw list (HACK)
	reticule_handle = 0;

#if defined(USE_SDL)
	// show the mouse cursor
	SDL_ShowCursor(SDL_ENABLE);
#endif

	return false;

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
#if defined(USE_SDL)
int SDL_main( int argc, char *argv[] )
#elif defined(WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main( int argc, char *argv[] )
#endif
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


//
// PLAY STATE
//

// player join
void PlayerJoinListener(unsigned int aId)
{
	// create player camera
	PlayerCamera *playercamera = new PlayerCamera(aId);
	Database::playercamera.Put(aId, playercamera);
	playercamera->Activate();

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

	if (PlayerCamera *playercamera = Database::playercamera.Get(aId))
	{
		// remove player camera
		delete playercamera;
		Database::playercamera.Delete(aId);
	}
}

// enter play state
void EnterPlayState()
{
	// clear the screen
	glClear(
		GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_TEST
		| GL_DEPTH_BUFFER_BIT
#endif
		);

#if defined(USE_SDL)
	// show the screen
	SDL_GL_SwapBuffers();
#elif defined(USE_SFML)
	window.Display();
#elif defined(USE_GLFW)
	glfwSwapBuffers();
#endif

	// reset camera position
	camerapos[0] = camerapos[1] = Vector2(0, 0);

	// reset simulation timer
	sim_rate = float(SIMULATION_RATE);
	sim_step = 1.0f / sim_rate;
	sim_turn = 0;
	sim_fraction = 1.0f;

	// input binding
	InitInput(INPUT_CONFIG.c_str());

	// add a join listener
	Database::playerjoin.Put(0xe28d61c6 /* "hud" */, PlayerJoinListener);

	// add a quit listener
	Database::playerquit.Put(0xe28d61c6 /* "hud" */, PlayerQuitListener);

	// level configuration
	if (!InitLevel(LEVEL_CONFIG.c_str()))
		setgamestate = STATE_SHELL;

#if defined(USE_SDL)
	// start audio
	SDL_PauseAudio(0);
#endif

	// create escape overlay
	Overlay *escape = new Overlay(0x9e212406 /* "escape" */);
	Database::overlay.Put(0x9e212406 /* "escape" */, escape);
	escape->SetAction(Overlay::Action(RenderEscapeOptions));

	// set to runtime mode
	runtime = true;

	DebugPrint("Simulating at %dHz (x%f)\n", SIMULATION_RATE, TIME_SCALE);
}

// run play state
// exit play state
void ExitPlayState()
{
	DebugPrint("Quitting...\n");

#if defined(USE_SDL)
	// stop audio
	SDL_PauseAudio(1);
#endif

	// stop any startup sound (HACK)
	StopSoundCue(0x94326baa /* "startup" */);

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

#ifdef GET_PERFORMANCE_DETAILS
class PerfTimer
{
public:
	static const int NUM_SAMPLES = 640;
	static LONGLONG mFrequency;
	static int mIndex;
	static int mCount;

public:
	static void Init()
	{
		// get frequency
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		mFrequency = freq.QuadPart;

		// reset counters
		mIndex = NUM_SAMPLES - 1;
		mCount = 0;
	}

	static void Next()
	{
		if (++mIndex >= NUM_SAMPLES)
			mIndex = 0;
		if (++mCount > NUM_SAMPLES)
			mCount = NUM_SAMPLES;
	}

public:
	LONGLONG mHistory[NUM_SAMPLES];
	LONGLONG mStamp;

public:
	PerfTimer()
	{
		// clear history
		memset(mHistory, 0, sizeof(mHistory));
	}

	void Clear()
	{
		mHistory[mIndex] = 0;
	}

	void Start()
	{
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		mStamp = count.QuadPart;
	}
	void Stop()
	{
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		mHistory[mIndex] += count.QuadPart - mStamp;
	}

	void Stamp()
	{
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		mHistory[mIndex] = count.QuadPart - mStamp;
		mStamp = count.QuadPart;
	}

	LONGLONG Ticks()
	{
		return mHistory[mIndex];
	}

	int Microseconds()
	{
		return int(1000000 * mHistory[mIndex] / mFrequency);
	}
};

LONGLONG PerfTimer::mFrequency;
int PerfTimer::mIndex;
int PerfTimer::mCount;
#endif

// common run state
void RunState()
{
#if defined(USE_SDL)
	// last ticks
	unsigned int ticks = SDL_GetTicks();
#elif defined(USE_SFML)
	// timer
	sf::Clock timer;

	// start timer
	timer.Reset();
#elif defined(USE_GLFW)
	double prevtime = glfwGetTime();
#endif

	// input logging
	TiXmlDocument inputlog(RECORD_CONFIG.c_str());
	TiXmlElement *inputlogroot;
	TiXmlElement *inputlognext;
	if (playback)
	{
		if (!inputlog.LoadFile())
			DebugPrint("error loading recording file \"%s\": %s\n", RECORD_CONFIG.c_str(), inputlog.ErrorDesc());
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
	PerfTimer::Init();

	PerfTimer control_timer;
	PerfTimer simulate_timer;
	PerfTimer collide_timer;
	PerfTimer update_timer;
	PerfTimer render_timer;
	PerfTimer overlay_timer;
	PerfTimer display_timer;
	PerfTimer total_timer;

	total_timer.Stamp();
#endif

#ifdef COLLECT_DEBUG_DRAW
	// create a new draw list
	GLuint debugdraw = glGenLists(1);
#endif

	// wait for user exit
	do
	{

#ifdef GET_PERFORMANCE_DETAILS
		PerfTimer::Next();

		control_timer.Clear();
		simulate_timer.Clear();
		collide_timer.Clear();
		update_timer.Clear();
		render_timer.Clear();
		overlay_timer.Clear();
		display_timer.Clear();
		total_timer.Clear();
#endif

		// INPUT PHASE

#if defined(USE_SDL)
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
		unsigned int nextticks = SDL_GetTicks();
		float delta = (nextticks - ticks) / 1000.0f;
		ticks = nextticks;
#elif defined(USE_SFML)
	    sf::Event event;
		while (window.GetEvent(event))
		{
			/* Give the console first dibs on event processing */
			if (OGLCONSOLE_SFMLEvent(&event))
				continue;

			// Some code for stopping application on close or when escape is pressed...
			switch (event.Type)
			{
			case sf::Event::Resized:
				glViewport(0, 0, event.Size.Width, event.Size.Height);
				break;
			case sf::Event::KeyPressed:
				input.OnPress( Input::TYPE_KEYBOARD, 0, event.Key.Code );
				switch(event.Key.Code)
				{
				case sf::Key::F4:
					if (event.Key.Alt)
						setgamestate = STATE_QUIT;
					break;
				case sf::Key::Return:
					if (event.Key.Alt)
					{
						SCREEN_FULLSCREEN = !SCREEN_FULLSCREEN;
						init_Window();
					}
					break;
				case sf::Key::Escape:
					if (curgamestate == STATE_PLAY)
					{
						if (escape)
							EscapeMenuExit();
						else
							EscapeMenuEnter();
					}
					break;
				case sf::Key::Pause:
					if (event.Key.Shift)
					{
						paused = true;
						singlestep = true;
					}
					else
					{
						paused = !paused;
					}
					window.ShowMouseCursor(paused);
					//SDL_WM_GrabInput(paused ? SDL_GRAB_OFF : SDL_GRAB_ON);
					//SDL_PauseAudio(paused);
					break;
				}
				break;
			case sf::Event::KeyReleased:
				input.OnRelease( Input::TYPE_KEYBOARD, 0, event.Key.Code );
				break;
			case sf::Event::MouseMoved:
				input.OnAxis( Input::TYPE_MOUSE_AXIS, 0, 0, float(int(event.MouseMove.X) * 2 - SCREEN_WIDTH) / float(SCREEN_HEIGHT) );
				input.OnAxis( Input::TYPE_MOUSE_AXIS, 0, 1, float(int(event.MouseMove.Y) * 2 - SCREEN_HEIGHT) / float(SCREEN_HEIGHT) );
				break;
			case sf::Event::MouseButtonPressed:
				input.OnPress( Input::TYPE_MOUSE_BUTTON, 0, event.MouseButton.Button );
				break;
			case sf::Event::MouseButtonReleased:
				input.OnRelease( Input::TYPE_MOUSE_BUTTON, 0, event.MouseButton.Button );
				break;
			case sf::Event::JoyMoved:
				input.OnAxis( Input::TYPE_JOYSTICK_AXIS, event.JoyMove.JoystickId, event.JoyMove.Axis, event.JoyMove.Position / 100.0f );
				break;
			case sf::Event::JoyButtonPressed:
				input.OnPress( Input::TYPE_JOYSTICK_BUTTON, event.JoyButton.JoystickId, event.JoyButton.Button );
				break;
			case sf::Event::JoyButtonReleased:
				input.OnRelease( Input::TYPE_JOYSTICK_BUTTON, event.JoyButton.JoystickId, event.JoyButton.Button );
				break;
			case sf::Event::Closed:
				setgamestate = STATE_QUIT;
				break;
			}
		}

		// get loop time in ticks
		float delta = timer.GetElapsedTime();
		timer.Reset();
		//ticks += delta;
#elif defined(USE_GLFW)
		if (glfwGetJoystickParam(0, GLFW_PRESENT))
		{
			// get joystick axis positions
			int axiscount = glfwGetJoystickParam(0, GLFW_AXES);
			float *axis = static_cast<float *>(_alloca(axiscount * sizeof(float)));
			axiscount = glfwGetJoystickPos(0, axis, axiscount);
			for (int i = 0; i < axiscount; ++i)
				input.OnAxis(Input::TYPE_JOYSTICK_AXIS, 0, i, axis[i]);

			// get joystick button states
			int buttoncount = glfwGetJoystickParam(0, GLFW_BUTTONS);
			unsigned char *button = static_cast<unsigned char *>(_alloca(buttoncount * sizeof(unsigned char)));
			buttoncount = glfwGetJoystickButtons(0, button, buttoncount);
			for (int i = 0; i < buttoncount; ++i)
				input.OnAxis(Input::TYPE_JOYSTICK_BUTTON, 0, i, button[i]);
		}
		
		double nexttime = glfwGetTime();
		float delta = float(nexttime - prevtime);
		prevtime = nexttime;
#endif

		// clamp ticks to something sensible
		// (while debugging, for example)
		if (delta > 0.1f)
			delta = 0.1f;

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
			frame_time = delta * TIME_SCALE;
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
#ifdef ENABLE_DEPTH_TEST
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
				Random::Seed(0x92D68CA2 ^ sim_turn);
				(void)Random::Int();

				// update database
				Database::Update();

				if (curgamestate == STATE_PLAY)
				{
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
							input.Playback(inputlognext);

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
							input.Record(&item, prev);

							// add the new input entry
							inputlogroot->InsertEndChild(item);
						}
					}
					else
					{
						// update input values
						input.Update();
					}
				}

				// do any pending turn actions
				DoTurn();


				// CONTROL PHASE

#ifdef GET_PERFORMANCE_DETAILS
				control_timer.Start();
#endif

				// control all entities
				Controller::ControlAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				control_timer.Stop();

				simulate_timer.Start();
#endif

				// SIMULATION PHASE
				// (generate forces)
				Simulatable::SimulateAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				simulate_timer.Stop();

				collide_timer.Start();
#endif

				// COLLISION PHASE
				// (apply forces and update positions)
				Collidable::CollideAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				collide_timer.Stop();

				update_timer.Start();
#endif

				// UPDATE PHASE
				// (use updated positions)
				Updatable::UpdateAll(sim_step);

#ifdef GET_PERFORMANCE_DETAILS
				update_timer.Stop();
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
			FloatInt floatint;
			floatint.f = sim_fraction;
			Random::Seed(0x92D68CA2 ^ sim_turn ^ floatint.u);
			(void)Random::Int();

#ifdef PRINT_SIMULATION_TIMER
			DebugPrint("delta=%f ticks=%d sim_t=%f\n", delta, ticks, sim_fraction);
#endif

#ifdef GET_PERFORMANCE_DETAILS
			render_timer.Start();
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
			render_timer.Stop();
#endif
		}

#ifdef GET_PERFORMANCE_DETAILS
		render_timer.Start();
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
		render_timer.Stop();

		overlay_timer.Start();
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
		glLoadIdentity();
		glOrtho(0, 640, 480, 0, -1, 1);

		// use 640x480 screen coordinates
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// render all overlays
		Overlay::RenderAll();

#ifdef GET_PERFORMANCE_DETAILS
		overlay_timer.Stop();

		if (!OPENGL_SWAPCONTROL)
		{
			display_timer.Start();

			// wait for rendering to finish
			glFinish();

			display_timer.Stop();
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
				{ control_timer.mHistory,	1.0f,	0.0f,	0.0f,	0.5f },
				{ simulate_timer.mHistory,	1.0f,	1.0f,	0.0f,	0.5f },
				{ collide_timer.mHistory,	0.0f,	1.0f,	0.0f,	0.5f },
				{ update_timer.mHistory,	0.0f,	0.5f,	1.0f,	0.5f },
				{ render_timer.mHistory,	1.0f,	0.0f,	1.0f,	0.5f },
				{ overlay_timer.mHistory,	1.0f,	0.5f,	0.0f,	0.5f },
				{ display_timer.mHistory,	0.5f,	0.5f,	0.5f,	0.5f },
			};

			// generate y samples
			float sample_y[SDL_arraysize(band_info)+1][PerfTimer::NUM_SAMPLES];
			int index = PerfTimer::mIndex;
			for (int i = 0; i < PerfTimer::NUM_SAMPLES; ++i)
			{
				float y = 480.0f;
				sample_y[0][i] = y;
				for (int band = 0; band < SDL_arraysize(band_info); ++band)
				{
					y -= 60.0f * 480.0f * band_info[band].time[index] / PerfTimer::mFrequency;
					sample_y[band+1][i] = y;
				}
				if (++index >= PerfTimer::NUM_SAMPLES)
					index = 0;
			}

			glBegin(GL_QUADS);
			for (int band = 0; band < SDL_arraysize(band_info); ++band)
			{
				glColor4fv(&band_info[band].r);
				float x = 0;
				float dx = 640.0f / PerfTimer::NUM_SAMPLES;
				for (int i = 0; i < PerfTimer::NUM_SAMPLES; i++)
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
				control_timer.Microseconds(),
				simulate_timer.Microseconds(),
				collide_timer.Microseconds(),
				update_timer.Microseconds(),
				render_timer.Microseconds(),
				overlay_timer.Microseconds(),
				display_timer.Microseconds());
		}
#endif

		// update frame timer
		total_timer.Stamp();

#if defined(PRINT_PERFORMANCE_FRAMERATE) || defined(DRAW_PERFORMANCE_FRAMERATE)
		if (FRAMERATE_OUTPUTSCREEN || FRAMERATE_OUTPUTPRINT)
		{
			// compute minimum, maximum, and average frame times over the past second
			LONGLONG total_min = LLONG_MAX;
			LONGLONG total_max = LLONG_MIN;
			LONGLONG total_sum = 0;
			LONGLONG total_samples = 0;
			int i = PerfTimer::mIndex;
			do
			{
				total_min = std::min(total_min, total_timer.mHistory[i]);
				total_max = std::max(total_max, total_timer.mHistory[i]);
				total_sum += total_timer.mHistory[i];
				++total_samples;
				i = (i > 0) ? i - 1 : PerfTimer::NUM_SAMPLES - 1;
			}
			while (total_sum <= PerfTimer::mFrequency && i != PerfTimer::mIndex && total_samples != PerfTimer::mCount);
			total_sum /= total_samples;

			// compute frame rates
			double rate_max = (double)PerfTimer::mFrequency / total_min;
			double rate_avg = (double)PerfTimer::mFrequency / total_sum;
			double rate_min = (double)PerfTimer::mFrequency / total_max;

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

#if defined(USE_SDL)
		/* Render our console */
		OGLCONSOLE_Draw();

		// show the screen
		SDL_GL_SwapBuffers();
#elif defined(USE_SFML)
		window.Display();
#elif defined(USE_GLFW)
		glfwSwapBuffers();
#endif

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