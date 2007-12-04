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

// Box2D includes
#include "Box2D.h"

// 2d math
#include "Vector2.h"
#include "Sphere2.h"
#include "Matrix2.h"
#include "AlignedBox2.h"

// utility includes
#include "Hash.h"
#include "Database.h"

// arena attributes
const float ARENA_X_MIN = -2048;
const float ARENA_X_MAX = 2048;
const float ARENA_Y_MIN = -2048;
const float ARENA_Y_MAX = 2048;

// collision layers
const int COLLISION_LAYER_PLAYER_SHIP = 0;
const int COLLISION_LAYER_PLAYER_BULLET = 1;

// debug function
extern int DebugPrint(const char *format, ...);

// loader
void ProcessDrawItems(TiXmlElement *element);

// configuration
//#define ENABLE_DEPTH_TEST
#define ENABLE_FOG

// for consistency between AA modes
#define ENABLE_SRC_ALPHA_SATURATE
#define DRAW_FRONT_TO_BACK
