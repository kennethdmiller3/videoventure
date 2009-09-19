// Core.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

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

#define _USE_MATH_DEFINES


#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif

// standard C library includes
#include <stdio.h>
#include <math.h>
#include <malloc.h>

// STL includes
#include <vector>
#include <deque>
#include <algorithm>
#include <cstdarg>

// Fast floating-point
#include "xs_Float.h"

#if defined(USE_SDL)
// SDL includes
#include "SDL.h"
#include "SDL_opengl.h"
#elif defined(USE_SFML)
// SFML includes
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <GL/gl.h>
#include <GL/glu.h>
// GLFW includes
#elif defined(USE_GLFW)
#include <GL/glfw.h>
#endif

// TinyXML includes
#include "tinyxml.h"

// Box2D includes
#include "Box2D.h"

// FastDelegate includes
#include "FastDelegate.h"

// 2d math
#include "Vector2.h"
#include "Sphere2.h"
#include "Matrix2.h"
#include "Transform2.h"
#include "AlignedBox2.h"

// utility includes
#include "Hash.h"
#include "Database.h"
#include "Input.h"
#include "Sound.h"


// UTILITY

// debug output
extern bool DEBUGPRINT_OUTPUTCONSOLE;
extern bool DEBUGPRINT_OUTPUTDEBUG;
extern bool DEBUGPRINT_OUTPUTSTDERR;

// debug print
extern int DebugPrint(const char *format, ...);

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
struct Color4
{
	float r;
	float g;
	float b;
	float a;

	Color4()
	{
	}

	Color4(float r, float g, float b, float a)
		: r(r), g(g), b(b), a(a)
	{
	}

	operator float *()
	{
		return static_cast<float *>(&r);
	}

	operator const float *() const
	{
		return static_cast<const float *>(&r);
	}
};

#ifndef SDL_arraysize
#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))
#endif
