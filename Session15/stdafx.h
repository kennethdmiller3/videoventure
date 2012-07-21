// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define GAME_API __declspec(dllimport)

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

// fast floating-point
#include "xs_Float.h"

// TinyXML includes
#include "tinyxml2.h"

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


// CONFIGURATION

#define USE_POOL_ALLOCATOR

#ifdef USE_POOL_ALLOCATOR
#include "MemoryPool.h"
#endif

