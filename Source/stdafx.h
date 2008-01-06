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

// loader
void ProcessTemplateItem(TiXmlElement *element, unsigned int template_id);
void ProcessTemplateItems(TiXmlElement *element);
void ProcessEntityItems(TiXmlElement *element);
void ProcessDrawItem(TiXmlElement *element, std::vector<unsigned int> &buffer);
void ProcessDrawItems(TiXmlElement *element, std::vector<unsigned int> &buffer);
void ExecuteDrawItems(const unsigned int buffer[], size_t count, float param);

// configuration
//#define ENABLE_DEPTH_TEST
#define ENABLE_FOG

// for consistency between AA modes
#define ENABLE_SRC_ALPHA_SATURATE
#define DRAW_FRONT_TO_BACK

#define USE_POOL_ALLOCATOR
