// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

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


// TO DO: move all these definitions to more appropriate locations


// debug function
extern int DebugPrint(const char *format, ...);


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
extern float frame_time;
extern float frame_turns;

// simulation values
extern float sim_rate;
extern float sim_step;
extern unsigned int sim_turn;
extern float sim_fraction;

// camera position
extern Vector2 camerapos[2];

// reticule handle (HACK)
extern GLuint reticule_handle;



// UTILITY

// cast anything to anything
template <typename O, typename I> inline O Cast(I i)
{
	union { I i; O o; } cast;
	cast.i = i;
	return cast.o;
}

// access float as integer
union FloatInt
{
	float f;
	int i;
	unsigned u;
};

// fast reciprocal square root
inline float InvSqrt(float x)
{
	float xhalf = 0.5f*x;
	FloatInt floatint;
	floatint.f = x;	// get bits for floating value
	floatint.i = 0x5f375a86 - (floatint.i >> 1); // gives initial guess y0
	floatint.f *= (1.5f-xhalf*floatint.f*floatint.f); // Newton step, repeating increases accuracy
	return floatint.f;
}

// linear interpolation
template<typename T> inline const T Lerp(T v0, T v1, float s)
{
	return (1 - s) * v0 + s * v1;
}

// value clamp
template<typename T> inline const T Clamp(T v, T min, T max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;
	return v;
}

// rectangle template
template<typename T> struct Rect
{
	T x;
	T y;
	T w;
	T h;
};

namespace Random
{
	// TO DO: allow multiple random number generators

	// random seed
	extern unsigned int gSeed;

	// set seed
	inline void Seed(unsigned int aSeed)
	{
		gSeed = aSeed;
	}

	// random unsigned long
	inline unsigned int Int()
	{
	//	gSeed = 1664525L * gSeed + 1013904223L;
		gSeed ^= (gSeed << 13);
		gSeed ^= (gSeed >> 17);
		gSeed ^= (gSeed << 5);
		return gSeed;
	}

	// random uniform float
	inline float Float()
	{
		FloatInt floatint;
		floatint.u = 0x3f800000 | (Int() >> 9);
		return floatint.f - 1.0f;
	}

	// random range value
	inline float Value(float aAverage, float aVariance)
	{
		return (2.0f * Float() - 1.0f) * aVariance + aAverage;
	}
}

// color typedef (HACK)
typedef Color4 Color4_2[2];


// queue a turn action
extern void OnTurn(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction);

#ifndef SDL_arraysize
#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))
#endif


// CONFIGURATION

//#define ENABLE_DEPTH_TEST
#define ENABLE_FOG

// for consistency between AA modes
//#define ENABLE_SRC_ALPHA_SATURATE
//#define DRAW_FRONT_TO_BACK

#define USE_POOL_ALLOCATOR
#define USE_CHANGE_DYNAMIC_TYPE

#define COLLECT_DEBUG_DRAW
//#define COLLIDABLE_DEBUG_DRAW

const int AUDIO_FREQUENCY = 48000;


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>
#endif

namespace std
{
	struct RTTI_NOT_SUPPORTED;
	typedef RTTI_NOT_SUPPORTED type_info;
}
#define typeid *( ::std::type_info* )sizeof 
#include <boost/variant.hpp>

