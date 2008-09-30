// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define _USE_MATH_DEFINES

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif

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

// debug function
extern int DebugPrint(const char *format, ...);

// input system
extern Input input;

// simulation values
extern float sim_rate;
extern float sim_step;
extern unsigned int sim_turn;
extern float sim_fraction;

// fast reciprocal square root
inline float InvSqrt(float x)
{
	float xhalf = 0.5f*x;
	int i = *(int*)&x; // get bits for floating value
	i = 0x5f375a86- (i>>1); // gives initial guess y0
	x = *(float*)&i; // convert bits back to float
	x = x*(1.5f-xhalf*x*x); // Newton step, repeating increases accuracy
	return x;
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

// random unsigned long
// TO DO: allow multiple random number generators
extern unsigned long randlongseed;
inline unsigned long RandLong()
{
//	randlongseed = 1664525L * randlongseed + 1013904223L;
	randlongseed ^= (randlongseed << 13);
	randlongseed ^= (randlongseed >> 17);
	randlongseed ^= (randlongseed << 5);
	return randlongseed;
}

// random uniform float
inline float RandFloat()
{
	unsigned long itemp = 0x3f800000 | (RandLong() >> 9);
	return (*(float *)&itemp)-1.0f;
}

// random range value
inline float RandValue(float aAverage, float aVariance)
{
	return (2.0f * RandFloat() - 1.0f) * aVariance + aAverage;
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
typedef Color4 Color4_2[2];


// queue a turn action
extern void OnTurn(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction);

#ifndef SDL_arraysize
#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))
#endif

// configuration
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