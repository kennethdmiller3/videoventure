#pragma once

#include "Expression.h"
#include "ExpressionSchema.h"
#include "ExpressionConfigure.h"
#include "ExpressionTime.h"
#include "Interpolator.h"

//
// INTERPOLATOR EXPRESSION
// returns interpolated value based on parameter
//

// apply keyframe interpolator (typed)
template <typename T> inline T EvaluateApplyInterpolator(int aCount, const float aKeys[], float aTime, int &aHint)
{
	T value = T();
	ApplyInterpolator(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), aCount, aKeys, aTime, aHint);
	return value;
}

// apply keyframe constant (typed)
template <typename T> inline T EvaluateApplyInterpolatorConstant(int aCount, const float aKeys[], float aTime, int &aHint)
{
	T value = T();
	ApplyInterpolatorConstant(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), aCount, aKeys, aTime, aHint);
	return value;
}

// apply interpolator (specialization for scalar)
template<> GAME_API float EvaluateApplyInterpolator<float>(int aCount, const float aKeys[], float aTime, int &aHint);

// apply interpolator (specialization for scalar)
template<> GAME_API float EvaluateApplyInterpolatorConstant<float>(int aCount, const float aKeys[], float aTime, int &aHint);

// apply interpolator (specialization for SIMD)
template <> GAME_API __m128 EvaluateApplyInterpolator<__m128>(int aCount, const float aKeys[], float aTime, int &aHint);

// apply interpolator (specialization for SIMD)
template <> GAME_API __m128 EvaluateApplyInterpolatorConstant<__m128>(int aCount, const float aKeys[], float aTime, int &aHint);

// evaluate typed keyframe interpolator
template <typename T> const T EvaluateInterpolator(EntityContext &aContext)
{
	// get parameter value
	float aTime(Expression::Evaluate<float>(aContext));

	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get keyframe hint
	Database::Key hintkey((aContext.mStream - aContext.mBegin)*4);
	int &aHint = *reinterpret_cast<int *>(&aContext.mVars->Open(hintkey));

	// get keyframe data
	const int aCount = Expression::Read<int>(aContext);
	const float * __restrict aKeys = reinterpret_cast<const float * __restrict>(aContext.mStream);

	// get interpolated value
	T value = EvaluateApplyInterpolator<T>(aCount, aKeys, aTime, aHint);

	// close keyframe hint
	aContext.mVars->Close(hintkey);

	// advance stream
	aContext.mStream = end;

	// return value
	return value;
}

// evaluate typed keyframe constant
template <typename T> const T EvaluateInterpolatorConstant(EntityContext &aContext)
{
	// get parameter value
	float aTime(Expression::Evaluate<float>(aContext));

	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get keyframe hint
	Database::Key hintkey((aContext.mStream - aContext.mBegin)*4);
	int &aHint = *reinterpret_cast<int *>(&aContext.mVars->Open(hintkey));

	// get keyframe data
	const int aCount = Expression::Read<int>(aContext);
	const float * __restrict aKeys = reinterpret_cast<const float * __restrict>(aContext.mStream);

	// get interpolated value
	T value = EvaluateApplyInterpolatorConstant<T>(aCount, aKeys, aTime, aHint);

	// close keyframe hint
	aContext.mVars->Close(hintkey);

	// advance stream
	aContext.mStream = end;

	// return value
	return value;
}

// configure typed interpolator
template <typename T> void ConfigureInterpolator(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append an interpolator expression
#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s interpolator\n", Expression::Schema<T>::NAME);
#endif
	int interpolate = 1;
	element->QueryIntAttribute("interpolate", &interpolate);
	if (interpolate)
		Expression::Append(buffer, EvaluateInterpolator<T>);
	else
		Expression::Append(buffer, EvaluateInterpolatorConstant<T>);

	if (const TiXmlElement *param = element->FirstChildElement("param"))
	{
		// get param expression
		ConfigureExpressionRoot<float>(param, buffer, sScalarNames, sScalarDefault);
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
