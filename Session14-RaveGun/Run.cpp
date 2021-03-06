#include "StdAfx.h"

#include "GameState.h"

#include "TurnAction.h"
#include "Controller.h"
#include "Simulatable.h"
#include "Collidable.h"
#include "Updatable.h"
#include "Drawlist.h"
#include "Renderable.h"
#include "Overlay.h"
#include "Sound.h"

#include "oglconsole.h"

#include <time.h>


// console
extern OGLCONSOLE_Console console;

// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_CreateFont();
extern "C" void OGLCONSOLE_Resize(OGLCONSOLE_Console console);

// forward declaration
extern bool OpenWindow(void);
extern void CloseWindow(void);
extern void EscapeMenuExit(void);
extern void EscapeMenuEnter(void);

// simulation attributes
int SIMULATION_RATE = 60;
float TIME_SCALE = 1.0f;
bool FIXED_STEP = false;

// rendering attributes
int MOTIONBLUR_STEPS = 1;
float MOTIONBLUR_TIME = 1.0f/60.0f;

// visual profiler
bool PROFILER_OUTPUTSCREEN = false;
bool PROFILER_OUTPUTPRINT = false;

// frame rate indicator
bool FRAMERATE_OUTPUTSCREEN = false;
bool FRAMERATE_OUTPUTPRINT = false;

// debug draw
bool DEBUG_DRAW = false;

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

// pause state
bool paused = false;
bool singlestep = false;
bool escape = false;

// configuration options
#define GET_PERFORMANCE_DETAILS
#define PRINT_PERFORMANCE_DETAILS
#define DRAW_PERFORMANCE_DETAILS
#define PRINT_PERFORMANCE_FRAMERATE
#define DRAW_PERFORMANCE_FRAMERATE
//#define PRINT_SIMULATION_TIMER

#ifdef GET_PERFORMANCE_DETAILS
#include "PerfTimer.h"
#endif

static void Pause(void)
{
	Platform::ShowCursor(true);
	Platform::GrabInput(false);
	Sound::Pause();
}

static void Resume(void)
{
	Platform::ShowCursor(!reticule_handle);
	Platform::GrabInput(true);
	Sound::Resume();
}

#if defined(USE_SDL)
static void Screenshot(void)
{
	// allocate a pixel array
	unsigned char *pixels = static_cast<unsigned char *>(malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4));

	// read pixels from frame buffer
	for (int y = 0; y < SCREEN_HEIGHT; ++y)
		glReadPixels(0, SCREEN_HEIGHT - y - 1, SCREEN_WIDTH, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels + y * SCREEN_WIDTH * 4);

	// create an SDL surface from the pixel array
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    unsigned int rmask = 0xff000000;
    unsigned int gmask = 0x00ff0000;
    unsigned int bmask = 0x0000ff00;
    unsigned int amask = 0x00000000;
#else
    unsigned int rmask = 0x000000ff;
    unsigned int gmask = 0x0000ff00;
    unsigned int bmask = 0x00ff0000;
    unsigned int amask = 0x00000000;
#endif
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SCREEN_WIDTH * 4, rmask, gmask, bmask, amask);

	// generate a filename
	time_t rawtime;
	time( &rawtime );
	tm* timeinfo;
	timeinfo = localtime( &rawtime );
	char acTimeString[ 128 ] = "";
	strftime( acTimeString, 128, "%Y_%m_%d_%H_%M_%S", timeinfo );
	char acFileName[ 256 ] = "";
	sprintf( acFileName, "screenshot_%s.bmp", acTimeString );

	// save to a bitmap file
	if (SDL_SaveBMP(surface, acFileName) < 0)
		DebugPrint("error: %s\n", SDL_GetError());

	// free the surface
	SDL_FreeSurface(surface);

	// free the pixel array
	free(pixels);
}
#endif

#if defined(USE_GLFW)

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
				CloseWindow();
				SCREEN_FULLSCREEN = !SCREEN_FULLSCREEN;
				OpenWindow();
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
				Pause();
			else
				Resume();
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
						CloseWindow();
						SCREEN_FULLSCREEN = !SCREEN_FULLSCREEN;
						OpenWindow();
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

					if (paused)
						Pause();
					else
						Resume();
					break;

				case SDLK_PRINT:
					Screenshot();
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
						CloseWindow();
						SCREEN_FULLSCREEN = !SCREEN_FULLSCREEN;
						OpenWindow();
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

					if (paused)
						Pause();
					else
						Resume();
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
			glFrustum( -0.5*VIEW_SIZE*SCREEN_WIDTH/SCREEN_HEIGHT, 0.5*VIEW_SIZE*SCREEN_WIDTH/SCREEN_HEIGHT, 0.5f*VIEW_SIZE, -0.5f*VIEW_SIZE, 256.0f*1.0f, 256.0f*5.0f );

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
						bool changed = false;
						for (int i = 0; i < Input::NUM_LOGICAL; ++i)
						{
							if (input.output[i] != prev[i])
							{
								changed = true; break;
							}
						}
						if (changed)
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
			view.min.x = viewpos.x - VIEW_SIZE * 0.5f * SCREEN_WIDTH / SCREEN_HEIGHT;
			view.max.x = viewpos.x + VIEW_SIZE * 0.5f * SCREEN_WIDTH / SCREEN_HEIGHT;
			view.min.y = viewpos.y - VIEW_SIZE * 0.5f;
			view.max.y = viewpos.y + VIEW_SIZE * 0.5f;

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
		if (DEBUG_DRAW)
		{
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
		}
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

		/* Render our console */
		OGLCONSOLE_Draw();

		// show the back buffer
		Platform::Present();

#if 0
#ifdef GET_PERFORMANCE_DETAILS
		if (OPENGL_SWAPCONTROL)
#endif
		// wait for rendering to finish
		glFinish();
#endif

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
