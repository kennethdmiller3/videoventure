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

// apply interpolator (typed)
template <typename T> inline T EvaluateApplyInterpolator(int aCount, const float aKeys[], float aTime, int &aHint)
{
	T value = T();
	ApplyInterpolator(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), aCount, aKeys, aTime, aHint);
	return value;
}

// apply interpolator (specialization for scalar)
template<> float EvaluateApplyInterpolator<float>(int aCount, const float aKeys[], float aTime, int &aHint);

// apply interpolator (specialization for SIMD)
template <> __m128 EvaluateApplyInterpolator<__m128>(int aCount, const float aKeys[], float aTime, int &aHint);

// evaluate typed interpolator
template <typename T> const T EvaluateInterpolator(EntityContext &aContext)
{
	// get parameter value
	float aTime(Expression::Evaluate<float>(aContext));

	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get interpolator data
	const int aCount = Expression::Read<int>(aContext);
	const float * __restrict aKeys = reinterpret_cast<const float * __restrict>(aContext.mStream);
	int aHint = 0;

	// get interpolated value
	T value = EvaluateApplyInterpolator<T>(aCount, aKeys, aTime, aHint);

	// advance stream
	aContext.mStream = end;

	// return value
	return value;
}

// configure typed interpolator
template <typename T> void ConfigureInterpolator(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append an interpolator expression
	DebugPrint("%s interpolator\n", Expression::Schema<T>::NAME);
	Expression::Append(buffer, EvaluateInterpolator<T>);
	if (const TiXmlElement *param = element->FirstChildElement("param"))
		ConfigureExpressionRoot<float>(param, buffer, sScalarNames, sScalarDefault);
	else
		Expression::Append(buffer, EvaluateTime);

	// process interpolator data
	buffer.push_back(0);
	int start = buffer.size();
	ConfigureInterpolatorItem(element, buffer, sizeof(T)/sizeof(float), names, defaults);
	buffer[start - 1] = buffer.size() - start;
}
