#pragma once

// UTILITY

// cast anything to anything
template <typename O, typename I> inline O Cast(I i)
{
	union { I i; O o; } cast;
	cast.i = i;
	return cast.o;
}

// zero value
template<typename T> inline T Zero()
{
	return T(0);
}

// extend a float
template<typename T> inline T Extend(const float x)
{
	return T(x);
}

// reciprocal
inline float Rcp(const float x)
{
	return 1.0f / x;
}

// square root
// (for consistency with SIMD)
inline float Sqrt(const float x)
{
	return sqrtf(x);
}

// reciprocal square root
inline float InvSqrt(const float x)
{
	float xhalf = 0.5f*x;
	union { float f; int i; } floatint;
	floatint.f = x;	// get bits for floating value
	floatint.i = 0x5f375a86 - (floatint.i >> 1); // gives initial guess y0
	floatint.f *= (1.5f-xhalf*floatint.f*floatint.f); // Newton step, repeating increases accuracy
	return floatint.f;
}

// linear interpolation
template<typename T> inline T Lerp(const T v0, const T v1, const float s)
{
	return v0 + (v1 - v0) * s;
}

// absolute value
inline float Abs(const float x)
{
	return fabsf(x);
}

// minimum of two values
template<typename T> inline T Min(const T a, const T b)
{
	return a < b ? a : b;
};

// maximum of two values
template<typename T> inline T Max(const T a, const T b)
{
	return a > b ? a : b;
};

// clamp value between zero and one
template<typename T> inline T Clamp01(const T v)
{
	if (v < 0)
		return 0;
	if (v > 1)
		return 1;
	return v;
}

// clamp value between minimum and maximum
template<typename T> inline T Clamp(const T v, const T min, const T max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;
	return v;
}

// sign of a value
template<typename T> inline T Sign(const T v)
{
	return (v == 0) ? T(0) : ((v > 0) ? T(1) : T(-1));
}

// fractional part of a value
inline float Frac(const float v)
{
	return v - floorf(v);
}

// step function
inline float Step(const float e, const float v)
{
	return v < e ? 0.0f : 1.0f;
}

// smooth-step function
inline float SmoothStep(const float e0, const float e1, const float v)
{
	if (v <= e0) return 0.0f;
	if (v >= e1) return 1.0f;
	const float t = (v - e0) / (e1 - e0);
	return t * t * (3 - 2 * t);
}

// rectangle template
template<typename T> struct Rect
{
	T x;
	T y;
	T w;
	T h;
};
#ifndef SDL_arraysize
#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))
#endif

// SIMD utilities
// including specializations of the above functions for SIMD
#include "UtilitySIMD.h"

// POSIX names
#if defined _MSC_VER
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

