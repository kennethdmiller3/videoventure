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

static float ConfigureInterpolatorKeyItem(const TiXmlElement *element, InterpolatorTemplate &interpolator, float time, float scale, const char * const names[], const float values[])
{
	// get local time value
	float frame = 0.0f;
	element->QueryFloatAttribute("time", &frame);

	// add a new key
	float *key = interpolator.AddKey(time + frame * scale);
	memcpy(&key[1], interpolator.mCount > 1 ? interpolator.GetValues(interpolator.mCount - 2) : values, interpolator.mWidth*sizeof(float));

	// modify values
	for (int i = 0; i < interpolator.mWidth; i++)
	{
		element->QueryFloatAttribute(names[i], &key[1+i]);
	}

	// return the new key time
	return time + frame * scale;
}

static float ConfigureInterpolatorKeyItems(const TiXmlElement *element, InterpolatorTemplate &interpolator, float time, float scale, const char * const names[], const float values[])
{
	// get default duration
	float duration = 0.0f;

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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
				duration = std::max(duration, ConfigureInterpolatorKeyItem(child, interpolator, time, scale, names, values) - time);
			}
			break;

		case 0xc7441a0f /* "step" */:
			{
				ConfigureInterpolatorKeyItem(child, interpolator, time, 0, names, values);
				element->QueryFloatAttribute("time", &duration);
				time += duration;
			}
			break;
		}
	}

	// return the new key
	return time + duration;
}

bool ConfigureInterpolatorItem(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float data[])
{
	if (!element->FirstChildElement())
		return false;

	// temporary interpolator
	InterpolatorTemplate interpolator(width);

	// start at zero time
	float time = 0.0f;
	element->QueryFloatAttribute("time", &time);

	// get time scale
	float scale = 1.0f;
	element->QueryFloatAttribute("timescale", &scale);
	if (element->QueryFloatAttribute("rate", &scale) == TIXML_SUCCESS)
		scale = 1.0f / scale;

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x0691ea25 /* "constant" */:
			{
				// TO DO: process constant interpolator
			}
			break;

		case 0xd00594c0 /* "linear" */:
			{
				// process linear interpolator
				time = ConfigureInterpolatorKeyItems(child, interpolator, time, scale, names, data);
			}
			break;

		case 0x9c265311 /* "hermite" */:
			{
				// TO DO: process hermite interpolator
			}
			break;

		case 0x6815c86c /* "key" */:
			{
				ConfigureInterpolatorKeyItem(child, interpolator, time, scale, names, data);
			}
			break;

		case 0xc7441a0f /* "step" */:
			{
				ConfigureInterpolatorKeyItem(child, interpolator, time, 0, names, data);
				float duration = 0.0f;
				if (child->QueryFloatAttribute("time", &duration) == TIXML_SUCCESS)
					time += duration * scale;
			}
			break;
		}
	}

	// if not enough keys...
	if (interpolator.mCount < 2)
	{
		// add a final key
		float *key = interpolator.AddKey(time);
		if (interpolator.mCount == 1)
		{
			memcpy(&key[1], data, interpolator.mWidth*sizeof(float));
			key = interpolator.AddKey(time);
		}
		memcpy(&key[1], interpolator.GetValues(interpolator.mCount-2), interpolator.mWidth*sizeof(float));
	}

	// push into the buffer
	buffer.push_back(interpolator.mCount);
	for (int i = 0; i < interpolator.mCount * interpolator.mStride; i++)
		buffer.push_back(*reinterpret_cast<unsigned int *>(&interpolator.mKeys[i]));
	return true;
}

#pragma optimize( "t", on )
bool ApplyInterpolatorConstant(float aTarget[], int aWidth, int aCount, const float aKeys[], float aTime, int &aHint)
{
	int i0, i1;

	// get stride
	const int aStride = aWidth + 1;

	// get time of hint key
	const float tt = aKeys[aHint * aStride];

	// if requested time is earlier...
	if (aTime < tt)
	{
		// set lower bound to first key
		i0 = 0;
		if (aTime < aKeys[i0 * aStride])
			return false;

		// set upper bound to hint key
		i1 = aHint;
	}
	else
	{
		// set lower bound to hint key
		i0 = aHint;

		// set upper bound to last key
		i1 = aCount-1;
		if (aTime > aKeys[i1 * aStride])
			return false;
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

		// found!
		// get first value
		const float *key0 = &aKeys[iL * aStride + 1];
		for (int element = 0; element < aWidth; element++)
		{
			aTarget[element] = key0[element];
		}
		aHint = iL;
		return true;
	}

	// not found...
	return false;
}

bool ApplyInterpolator(float aTarget[], int aWidth, int aCount, const float aKeys[], float aTime, int &aHint)
{
	int i0, i1;

	// get stride
	const int aStride = aWidth + 1;

	// get time of hint key
	const float tt = aKeys[aHint * aStride];

	// if requested time is earlier...
	if (aTime < tt)
	{
		// set lower bound to first key
		i0 = 0;
		if (aTime < aKeys[i0 * aStride])
			return false;

		// set upper bound to hint key
		i1 = aHint;
	}
	else
	{
		// set lower bound to hint key
		i0 = aHint;

		// set upper bound to last key
		i1 = aCount-1;
		if (aTime > aKeys[i1 * aStride])
			return false;
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

		// found!
		// interpolate the value
		const float *key0 = &aKeys[iL * aStride + 1];
		const float *key1 = &aKeys[iH * aStride + 1];
		const float t = (aTime - tL) / (tH - tL + FLT_EPSILON);
		for (int element = 0; element < aWidth; element++)
		{
			aTarget[element] = Lerp(key0[element], key1[element], t);
		}
		aHint = iL;
		return true;
	}

	// not found...
	return false;
}
#pragma optimize( "", on )