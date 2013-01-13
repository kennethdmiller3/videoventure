#include "StdAfx.h"

#include "ExpressionInterpolator.h"
#include "ExpressionConfigure.h"

static Expression::Loader<float> interpolatorfloat(0x83588fd4 /* "interpolator" */, ConfigureInterpolator<float>);
static Expression::Loader<__m128> interpolatorvector(0x83588fd4 /* "interpolator" */, ConfigureInterpolator<__m128>);

// apply keyframe interpolator (typed)
template <typename T> inline T EvaluateApplyInterpolator(int aCount, const float aKeys[], float aTime, int &aHint);

// apply keyframe constant (typed)
template <typename T> inline T EvaluateApplyInterpolatorConstant(int aCount, const float aKeys[], float aTime, int &aHint);

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
	return Lerp(data0, data1, t);
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

// evaluate typed keyframe interpolator
template <typename T> T EvaluateInterpolator(EntityContext &aContext)
{
	// get parameter value
	float aTime(Expression::Evaluate<float>(aContext));

	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get keyframe hint
#ifdef EVALUATE_INTERPOLATOR_USE_HINT
	Database::Key hintkey(0x586a05fc /* "interpolator@" */ + (aContext.mStream - aContext.mBegin));
	int &aHint = *reinterpret_cast<int *>(&aContext.mVars->Open(hintkey));
#else
	int aHint = 0;
#endif

	// get keyframe data
	const int aCount = Expression::Read<int>(aContext);
	const float * __restrict aKeys = reinterpret_cast<const float * __restrict>(aContext.mStream);

	// get interpolated value
	T value = EvaluateApplyInterpolator<T>(aCount, aKeys, aTime, aHint);

#ifdef EVALUATE_INTERPOLATOR_USE_HINT
	// close keyframe hint
	aContext.mVars->Close(hintkey);
#endif

	// advance stream
	aContext.mStream = end;

	// return value
	return value;
}

// evaluate typed keyframe constant
template <typename T> T EvaluateInterpolatorConstant(EntityContext &aContext)
{
	// get parameter value
	float aTime(Expression::Evaluate<float>(aContext));

	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get keyframe hint
#ifdef EVALUATE_INTERPOLATOR_USE_HINT
	Database::Key hintkey(0x586a05fc /* "interpolator@" */ + (aContext.mStream - aContext.mBegin));
	int &aHint = *reinterpret_cast<int *>(&aContext.mVars->Open(hintkey));
#else
	int aHint = 0;
#endif

	// get keyframe data
	const int aCount = Expression::Read<int>(aContext);
	const float * __restrict aKeys = reinterpret_cast<const float * __restrict>(aContext.mStream);

	// get interpolated value
	T value = EvaluateApplyInterpolatorConstant<T>(aCount, aKeys, aTime, aHint);

#ifdef EVALUATE_INTERPOLATOR_USE_HINT
	// close keyframe hint
	aContext.mVars->Close(hintkey);
#endif

	// advance stream
	aContext.mStream = end;

	// return value
	return value;
}

// configure typed interpolator
template <typename T> void ConfigureInterpolator(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append an interpolator expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s interpolator\n", Expression::Schema<T>::NAME);
#endif
	bool interpolate = true;
	element->QueryBoolAttribute("interpolate", &interpolate);
	if (interpolate)
		Expression::Append(buffer, EvaluateInterpolator<T>);
	else
		Expression::Append(buffer, EvaluateInterpolatorConstant<T>);

	if (const tinyxml2::XMLElement *param = element->FirstChildElement("param"))
	{
		// get param expression
		Expression::Loader<float>::ConfigureRoot(param, buffer, sScalarNames, sScalarDefault);
	}
	else if (const char *input = element->Attribute("param"))
	{
		// attribute variable reference
		Expression::Append(buffer, EvaluateVariable<float>, Hash(input));
	}
	else
	{
		// default to time
		Expression::Append(buffer, EvaluateTime);
	}

	// process interpolator data
	buffer.push_back(0);
	int start = buffer.size();
	ConfigureInterpolatorItem(element, buffer, sizeof(T)/sizeof(float), names, defaults);
	buffer[start - 1] = buffer.size() - start;
}


template <> void ConfigureInterpolator<bool>(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
}
