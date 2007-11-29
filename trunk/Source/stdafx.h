// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <math.h>

// STL includes
#include <algorithm>
#include <list>
#include <vector>
#ifdef __GNUC__
#include <ext/hash_map>
namespace stdext = __gnu_cxx;
#else
#include <hash_map>
#endif

// SDL includes
#include "SDL.h"
#include "SDL_opengl.h"

// TinyXML includes
#include "tinyxml.h"

// 2d math
#include "Vector2.h"
#include "Sphere2.h"
#include "AlignedBox2.h"

// utility includes
#include "Hash.h"

// screen attributes
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// arena attributes
const int ARENA_X_MIN = -2048;
const int ARENA_X_MAX = 2048;
const int ARENA_Y_MIN = -2048;
const int ARENA_Y_MAX = 2048;

// collision layers
const int COLLISION_LAYER_PLAYER_SHIP = 0;
const int COLLISION_LAYER_PLAYER_BULLET = 1;

// debug function
extern int DebugPrint(const char *format, ...);

// configuration
#define ENABLE_MULTISAMPLING
#ifdef ENABLE_MULTISAMPLING
const int MULTISAMPLE_BUFFERS = 1;
const int MULTISAMPLE_SAMPLES = 16;
#else
#define ENABLE_ANTIALIAS_POINT
#define ENABLE_ANTIALIAS_LINE
#define ENABLE_ANTIALIAS_POLYGON
#endif
//#define ENABLE_DEPTH_TEST
#define ENABLE_FOG

// for consistency between AA modes
#define ENABLE_SRC_ALPHA_SATURATE
#define DRAW_FRONT_TO_BACK
