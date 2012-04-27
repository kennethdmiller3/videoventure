#include "StdAfx.h"
#include "Interpolator.h"

InterpolatorTemplate::InterpolatorTemplate(int aWidth)
	: mWidth(aWidth), mStride(1 + aWidth), mCount(0), mKeys(NULL)
{
}

InterpolatorTemplate::~InterpolatorTemplate()
{
	free(mKeys);
}

float *InterpolatorTemplate::AddKey(float aParam)
{
	// add a new keyframe
	mKeys = static_cast<float *>(realloc(mKeys, (mCount + 1) * mStride * sizeof(float)));

	// get the new keyframe data
	float *data = GetKey(mCount++);

	// initalize keyframe data
	data[0] = aParam;

	// return the new keyframe data
	return data;
}

static float ConfigureInterpolatorKeyItem(const tinyxml2::XMLElement *element, InterpolatorTemplate &interpolator, float offset, float scale, float prev, const char * const names[], const float values[])
{
	// get key time
	float keytime = prev;
	element->QueryFloatAttribute("time", &keytime);

	// add a new key
	float *key = interpolator.AddKey(offset + scale * keytime);
	memcpy(&key[1], interpolator.mCount > 1 ? interpolator.GetValues(interpolator.mCount - 2) : values, interpolator.mWidth*sizeof(float));

	// modify values
	for (int i = 0; i < interpolator.mWidth; i++)
	{
		element->QueryFloatAttribute(names[i], &key[1+i]);
	}

	// apply key step
	float step;
	if (element->QueryFloatAttribute("step", &step) == tinyxml2::XML_SUCCESS)
		keytime += step;

	// return key time
	return keytime;
}

static float ConfigureInterpolatorKeyItems(const tinyxml2::XMLElement *element, InterpolatorTemplate &interpolator, float offset, float scale, const char * const names[], const float values[])
{
	// get default duration
	float duration = 0.0f;

	// last key time (relative to offset)
	float last = 0.0f;

	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
#if 0
		case 0x652b04df /* "start" */:
			{
				ConfigureInterpolatorKeyItem(child, interpolator, time, scale, names, values);
			}
			break;

		case 0x6a8e75aa /* "end" */:
			{
				element->QueryFloatAttribute("time", &duration);
				ConfigureInterpolatorKeyItem(child, interpolator, time + duration, scale, names, values);
			}
			break;
#endif

		case 0x6815c86c /* "key" */:
			{
				last = ConfigureInterpolatorKeyItem(child, interpolator, offset, scale, last, names, values);
				duration = std::max(duration, last);
			}
			break;
		}
	}

	// return the new offset
	return offset + duration * scale;
}

bool ConfigureInterpolatorItem(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float data[])
{
	if (!element->FirstChildElement())
		return false;

	// temporary interpolator
	InterpolatorTemplate interpolator(width);

	// get time offset
	float offset = 0.0f;
	element->QueryFloatAttribute("timeoffset", &offset);

	// get time scale
	float scale = 1.0f;
	element->QueryFloatAttribute("timescale", &scale);
	if (element->QueryFloatAttribute("rate", &scale) == tinyxml2::XML_SUCCESS)
		scale = 1.0f / scale;

	// process child elements
	float end = ConfigureInterpolatorKeyItems(element, interpolator, offset, scale, names, data);

	// if no key data...
	if (interpolator.mCount == 0)
	{
		// add first key
		float *key = interpolator.AddKey(offset);
		memcpy(&key[1], data, interpolator.mWidth*sizeof(float));
	}

	// if only one key, or the last key had duration...
	if (interpolator.mCount == 1 || interpolator.GetKey(interpolator.mCount-1)[0] < end)
	{
		// add last key
		float *key = interpolator.AddKey(end);
		memcpy(&key[1], interpolator.GetValues(interpolator.mCount-1), interpolator.mWidth*sizeof(float));
	}

	// push into the buffer
	buffer.reserve(buffer.size() + 1 + interpolator.mCount * interpolator.mStride);
	buffer.push_back(interpolator.mCount);
	for (int i = 0; i < interpolator.mCount * interpolator.mStride; i++)
		buffer.push_back(*reinterpret_cast<unsigned int *>(&interpolator.mKeys[i]));
	return true;
}

#pragma optimize( "t", on )

// binary search
int FindKeyIndex(const int aStride, const int aCount, const float aKeys[], const float aTime, const int aHint)
{
	int i0, i1;

	// get time of hint key
	const float tt = aKeys[aHint * aStride];

	// if requested time is earlier...
	if (aTime < tt)
	{
		// set lower bound to first key
		i0 = 0;

		// not found if earlier than first key
		if (aTime < aKeys[i0 * aStride])
			return -1;

		// set upper bound to hint key
		i1 = aHint;
	}
	else
	{
		// set lower bound to hint key
		i0 = aHint;

		// set upper bound to last key
		i1 = aCount-1;

		// not found if later than last key
		if (aTime > aKeys[i1 * aStride])
			return -1;
	}

	// while still checking a range of keys...
	while (i0 <= i1)
	{
		// get midpoint
		const int im = (i0 + i1) >> 1;

		// if time is before segment start...
		const int iL = im;
		const float tL = aKeys[iL * aStride];
		if (aTime < tL - FLT_EPSILON)
		{
			// set upper bound to segment start
			i1 = iL;
			continue;
		}

		// if time is after segment end...
		const int iH = im + 1;
		const float tH = aKeys[iH * aStride];
		if (aTime > tH + FLT_EPSILON)
		{
			// set lower bound to segment end
			i0 = iH;
			continue;
		}

		// return the index
		return iL;
	}

	// not found
	return -1;
}

bool ApplyInterpolatorConstant(float aTarget[], int aWidth, int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = aWidth + 1;

	// get the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return false;

	// update hint index
	aHint = index;

	// use the first value
	const float * __restrict key = aKeys + index * aStride;
	const float * __restrict data0 = &key[1];
	for (int element = 0; element < aWidth; element++)
	{
		aTarget[element] = data0[element];
	}
	return true;
}

bool ApplyInterpolator(float aTarget[], int aWidth, int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = aWidth + 1;

	// get the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return false;

	// update hint index
	aHint = index;

	// interpolate the value
	const float * __restrict key = aKeys + index * aStride;
	const float time0 = key[0];
	const float * __restrict data0 = &key[1];
	const float time1 = key[aStride];
	const float * __restrict data1 = &key[aStride + 1];
	const float t = (aTime - time0) / (time1 - time0 + FLT_EPSILON);
	for (int element = 0; element < aWidth; element++)
	{
		aTarget[element] = Lerp(data0[element], data1[element], t);
	}
	return true;
}
#pragma optimize( "", on )