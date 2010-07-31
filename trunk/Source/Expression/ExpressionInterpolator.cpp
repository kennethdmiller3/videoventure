#include "StdAfx.h"

#include "ExpressionInterpolator.h"
#include "ExpressionConfigure.h"

static ExpressionConfigure::Auto<float> interpolatorfloat(0x83588fd4 /* "interpolator" */, ConfigureInterpolator<float>);
static ExpressionConfigure::Auto<__m128> interpolatorvector(0x83588fd4 /* "interpolator" */, ConfigureInterpolator<__m128>);

// apply interpolator (specialization for scalar)
template<> float EvaluateApplyInterpolator<float>(int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = sizeof(float)/sizeof(float) + 1;

	// find the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return 0.0f;

	// interpolate the value
	const float * __restrict key = aKeys + index * aStride;
	const float time0 = key[0];
	const float data0 = key[1];
	const float time1 = key[aStride];
	const float data1 = key[aStride + 1];
	const float t = (aTime - time0) / (time1 - time0 + FLT_EPSILON);
	return data0 + (data1 - data0) * t;
}

// apply constant interpolator (specialization for scalar)
template<> float EvaluateApplyInterpolatorConstant<float>(int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = sizeof(float)/sizeof(float) + 1;

	// find the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return 0.0f;

	// interpolate the value
	const float * __restrict key = aKeys + index * aStride;
	const float data0 = key[1];
	return data0;
}

// apply interpolator (specialization for SIMD)
template <> __m128 EvaluateApplyInterpolator<__m128>(int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = sizeof(__m128)/sizeof(float) + 1;

	// find the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return _mm_setzero_ps();

	// interpolate the value
	const float * __restrict key = aKeys + index * aStride;
	const float time0 = key[0];
	const __m128 data0 = _mm_loadu_ps(&key[1]);
	const float time1 = key[aStride];
	const __m128 data1 = _mm_loadu_ps(&key[aStride + 1]);
	const float t = (aTime - time0) / (time1 - time0 + FLT_EPSILON);
	return _mm_add_ps(data0, _mm_mul_ps(_mm_sub_ps(data1, data0), _mm_set_ps1(t)));
}

// apply constant interpolator (specialization for SIMD)
template <> __m128 EvaluateApplyInterpolatorConstant<__m128>(int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = sizeof(__m128)/sizeof(float) + 1;

	// find the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return _mm_setzero_ps();

	// interpolate the value
	const float * __restrict key = aKeys + index * aStride;
	const __m128 data0 = _mm_loadu_ps(&key[1]);
	return data0;
}
