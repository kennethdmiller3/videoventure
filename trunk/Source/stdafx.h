// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define GAME_API __declspec(dllexport)

#include "targetver.h"

// SIMD intrinsics
#include "xmmintrin.h"

// standard C library includes
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <malloc.h>

// STL includes
#include <vector>
#include <deque>
#include <algorithm>

// platform header
#include "Platform.h"

// fast floating-point
#include "xs_Float.h"

// TinyXML includes
#include "tinyxml.h"

// Box2D includes
#include "Box2d/Box2D.h"

// FastDelegate includes
#include "FastDelegate.h"

// 2d math
#include "Vector2.h"
#include "Sphere2.h"
#include "Matrix2.h"
#include "Transform2.h"
#include "AlignedBox2.h"

// 3d math
#include "Vector3.h"

// vector 4
#include "Vector4.h"

// color
#include "Color4.h"

// utility includes
#include "Hash.h"
#include "Database.h"
#include "Input.h"
#include "Random.h"
#include "Utility.h"


// TO DO: move all these definitions to more appropriate locations


// debug function
extern GAME_API int DebugPrint(const char *format, ...);


// GLOBAL VALUES (HACK)

// screen attributes
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern int SCREEN_DEPTH;
extern bool SCREEN_FULLSCREEN;

// view attributes
extern float VIEW_SIZE;

// opengl attributes
extern bool OPENGL_SWAPCONTROL;
extern bool OPENGL_ANTIALIAS;
extern int OPENGL_MULTISAMPLE;

// debug output
extern bool DEBUGPRINT_OUTPUTCONSOLE;
extern bool DEBUGPRINT_OUTPUTDEBUG;
extern bool DEBUGPRINT_OUTPUTSTDERR;

// visual profiler
extern bool PROFILER_OUTPUTSCREEN;
extern bool PROFILER_OUTPUTPRINT;

// frame rate indicator
extern bool FRAMERATE_OUTPUTSCREEN;
extern bool FRAMERATE_OUTPUTPRINT;

// debug draw
extern bool DEBUG_DRAW;

// simulation attributes
extern int SIMULATION_RATE;
extern float TIME_SCALE;
extern bool FIXED_STEP;

// rendering attributes
extern int MOTIONBLUR_STEPS;
extern float MOTIONBLUR_TIME;

// sound attributes
extern int SOUND_CHANNELS;
extern float SOUND_VOLUME_EFFECT;
extern float SOUND_VOLUME_MUSIC;

// simulation attributes
extern int SIMULATION_RATE;

// default input configuration
extern std::string INPUT_CONFIG;

// default level configuration
extern std::string LEVEL_CONFIG;

// default record configuration
extern std::string RECORD_CONFIG;
extern bool record;
extern bool playback;

// runtime
extern bool runtime;

// device was reset (HACK)
extern bool wasreset;

// input system
extern Input input;

// frame values
extern GAME_API float frame_time;
extern GAME_API float frame_turns;

// simulation values
extern GAME_API float sim_rate;
extern GAME_API float sim_step;
extern GAME_API unsigned int sim_turn;
extern GAME_API float sim_fraction;

// camera position
extern GAME_API Vector2 camerapos[2];

// reticule handle (HACK)
extern GLuint reticule_handle;


// queue a turn action
extern void OnTurn(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction);


// CONFIGURATION

//#define ENABLE_DEPTH_TEST
#define ENABLE_FOG

// for consistency between AA modes
#define ENABLE_SRC_ALPHA_SATURATE
#define DRAW_FRONT_TO_BACK

#define USE_POOL_ALLOCATOR
#define USE_CHANGE_DYNAMIC_TYPE

#define COLLECT_DEBUG_DRAW
//#define COLLIDABLE_DEBUG_DRAW

const int AUDIO_FREQUENCY = 48000;


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>
#endif
