#include "StdAfx.h"
#include "Interpolator.h"

namespace Database
{
	Typed<Typed<InterpolatorTemplate> > interpolatortemplate(0x9074abfe /* "interpolatortemplate" */);
}

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

static float ProcessInterpolatorKeyItem(const TiXmlElement *element, InterpolatorTemplate &interpolator, float time, float scale, const char *names[], const float values[])
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

static float ProcessInterpolatorKeyItems(const TiXmlElement *element, InterpolatorTemplate &interpolator, float time, float scale, const char *names[], const float values[])
{
	// get default duration
	float duration = 0.0f;

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x652b04df /* "start" */:
			{
				ProcessInterpolatorKeyItem(child, interpolator, time, scale, names, values);
			}
			break;

		case 0x6a8e75aa /* "end" */:
			{
				element->QueryFloatAttribute("time", &duration);
				ProcessInterpolatorKeyItem(child, interpolator, time + duration, scale, names, values);
			}
			break;

		case 0x6815c86c /* "key" */:
			{
				duration = std::max(duration, ProcessInterpolatorKeyItem(child, interpolator, time, scale, names, values) - time);
			}
			break;
		}
	}

	// return the new key
	return time + duration;
}

void ProcessInterpolatorItem(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char *names[], const float data[])
{
	if (!element->FirstChildElement())
		return;

	// temporary interpolator
	InterpolatorTemplate interpolator(width);

	// start at zero time
	float time = 0.0f;

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
				time = ProcessInterpolatorKeyItems(child, interpolator, time, scale, names, data);
			}
			break;

		case 0x9c265311 /* "hermite" */:
			{
				// TO DO: process hermite interpolator
			}
			break;

		case 0x6815c86c /* "key" */:
			{
				ProcessInterpolatorKeyItem(child, interpolator, time, scale, names, data);
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
}

bool InterpolatorTemplate::Apply(float aTarget[], float aTime, int &aIndex)
{
	// while the time is less than the start time for the current index...
	while (aTime < mKeys[aIndex * mStride])
	{
		// quit if out of range
		if (aIndex <= 0)
			return false;

		// go to the previous index
		--aIndex;
	}

	// while the time is greater than the end time for the current index...
	while (aTime > mKeys[(aIndex + 1) * mStride])
	{
		// quit if out of range
		if (aIndex >= mCount - 1)
			return false;

		// go to the next index
		++aIndex;
	}

	// interpolate the value
	float *key0 = mKeys + aIndex * mStride;
	float *key1 = key0 + mStride;
	float t = (aTime - key0[0]) / (key1[0] - key0[0]);
	for (int element = 0; element < mWidth; element++)
	{
		aTarget[element] = Lerp(key0[1 + element], key1[1 + element], t);
	}
	return true;
}
