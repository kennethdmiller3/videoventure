#pragma once

// UTILITY

// cast anything to anything
template <typename O, typename I> inline O Cast(I i)
{
	union { I i; O o; } cast;
	cast.i = i;
	return cast.o;
}

// fast reciprocal square root
inline float InvSqrt(float x)
{
	float xhalf = 0.5f*x;
	union { float f; int i; } floatint;
	floatint.f = x;	// get bits for floating value
	floatint.i = 0x5f375a86 - (floatint.i >> 1); // gives initial guess y0
	floatint.f *= (1.5f-xhalf*floatint.f*floatint.f); // Newton step, repeating increases accuracy
	return floatint.f;
}

// linear interpolation
template<typename T> inline T Lerp(T v0, T v1, float s)
{
	return v0 + (v1 - v0) * s;
}

// value clamp
template<typename T> inline T Clamp(T v, T min, T max)
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

#ifndef SDL_arraysize
#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))
#endif
