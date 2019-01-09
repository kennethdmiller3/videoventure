#pragma once

// SSE UTILITY

#ifdef _MSC_VER
// define arithmetic operators for Visual C++
// (Clang and GCC already define these)
static inline __m128 operator+(const __m128 a, const __m128 b)
{
	return _mm_add_ps(a,b);
}
static inline __m128 operator-(const __m128 a, const __m128 b)
{
	return _mm_sub_ps(a,b);
}
static inline __m128 operator*(const __m128 a, const __m128 b)
{
	return _mm_mul_ps(a,b);
}
static inline __m128 operator/(const __m128 a, const __m128 b)
{
	return _mm_div_ps(a,b);
}
static inline __m128 operator-(const __m128 a)
{
	//return _mm_sub_ps(_mm_setzero_ps(), a);
	return _mm_xor_ps(a, _mm_set_ps1(-0.0f));
}
#endif

// SSE overloads of functions in Utility.h

// zero value
template<> inline __m128 Zero<__m128>()
{
	return _mm_setzero_ps();
}

// extend a float
template<> inline __m128 Extend<__m128>(const float x)
{
	return _mm_set_ps1(x);
}

// reciprocal
inline __m128 Rcp(const __m128 x)
{
	return _mm_rcp_ps(x);
}

// square root
inline __m128 Sqrt(const __m128 x)
{
	return _mm_sqrt_ps(x);
}

// reciprocal square root
inline __m128 InvSqrt(const __m128 x)
{
	return _mm_rsqrt_ps(x);
}

// linear interpolation
template<> inline __m128 Lerp<__m128>(const __m128 v0, const __m128 v1, const float s)
{
	return _mm_add_ps(v0, _mm_mul_ps(_mm_sub_ps(v1, v0), _mm_set_ps1(s)));
}

// absolute value
inline __m128 Abs(const __m128 x)
{
	//return _mm_max_ps(arg1, _mm_sub_ps(_mm_setzero_ps(), arg1));
	return _mm_andnot_ps(x, _mm_set_ps1(-0.0f));
}

// minimum of two values
template<> inline __m128 Min<__m128>(const __m128 a, const __m128 b)
{
  return _mm_min_ps(a,b);
}

// maximum of two values
template<> inline __m128 Max<__m128>(const __m128 a, const __m128 b)
{
  return _mm_max_ps(a,b);
}

// clamp value between zero and one
template<> inline __m128 Clamp01<__m128>(const __m128 v)
{
	return _mm_min_ps(_mm_max_ps(v, _mm_setzero_ps()), _mm_set_ps1(1));
}

// clamp value between minimum and maximum
template<> inline __m128 Clamp<__m128>(const __m128 v, const __m128 min, const __m128 max)
{
	return _mm_min_ps(_mm_max_ps(v, min), max);
}

// step function
inline __m128 Step(const __m128 e, const __m128 v)
{
	return _mm_and_ps(_mm_cmpgt_ps(v, e), _mm_set_ps1(1));
}

// smooth-step function
inline __m128 SmoothStep(const __m128 e0, const __m128 e1, const __m128 v)
{
	const __m128 s(_mm_min_ps(_mm_max_ps(v, e0), e1));
	const __m128 t(_mm_div_ps(_mm_sub_ps(s, e0), _mm_sub_ps(e1, e0)));
	return _mm_mul_ps(t, _mm_mul_ps(t, _mm_sub_ps(_mm_set_ps1(3), _mm_add_ps(t, t))));
}


// truncate to integer
inline __m128i TruncateToInt(const __m128 v)
{
	return _mm_cvttps_epi32(v);
}

// round to integer
inline __m128i RoundToInt(const __m128 v)
{
	return _mm_srai_epi32(_mm_cvtps_epi32(_mm_add_ps(_mm_add_ps(v, v), _mm_set_ps1(0.5f))), 1);
}

// floor to integer
inline __m128i FloorToInt(const __m128 v)
{
	return _mm_srai_epi32(_mm_cvtps_epi32(_mm_sub_ps(_mm_add_ps(v, v), _mm_set_ps1(0.5f))), 1);
}

// ceiling to integer
inline __m128i CeilToInt(const __m128 v)
{
	return _mm_srai_epi32(_mm_sub_epi32(_mm_setzero_si128(), _mm_cvtps_epi32(_mm_sub_ps(_mm_set_ps1(-0.5f), _mm_add_ps(v, v)))), 1);
}
