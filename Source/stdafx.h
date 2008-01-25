// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>

// STL includes
#include <list>
#include <vector>
#include <map>
#include <deque>

// SDL includes
#include "SDL.h"
#include "SDL_opengl.h"

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
#include "AlignedBox2.h"

// utility includes
#include "Hash.h"
#include "Database.h"
#include "Input.h"

// arena attributes
const float ARENA_X_MIN = -2048;
const float ARENA_X_MAX = 2048;
const float ARENA_Y_MIN = -2048;
const float ARENA_Y_MAX = 2048;

// debug function
extern int DebugPrint(const char *format, ...);

// input system
extern Input input;

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
inline float Lerp(float v0, float v1, float s)
{
	return v0 + (v1 - v0) * s;
}

// loader
void ProcessTemplateItem(const TiXmlElement *element, unsigned int template_id);
void ProcessTemplateItems(const TiXmlElement *element);
void ProcessEntityItems(const TiXmlElement *element);

// configuration
//#define ENABLE_DEPTH_TEST
#define ENABLE_FOG

// for consistency between AA modes
#define ENABLE_SRC_ALPHA_SATURATE
#define DRAW_FRONT_TO_BACK

#define USE_POOL_ALLOCATOR
#define USE_CHANGE_DYNAMIC_TYPE

const int AUDIO_FREQUENCY = 44100;