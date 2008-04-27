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
	int i0, i1;
	float t0, t1;

	// get time of saved index key
	float tt = mKeys[aIndex * mStride];

	// if requested time is earlier...
	if (aTime < tt)
	{
		// set lower bound to first key
		i0 = 0;
		t0 = mKeys[i0 * mStride];
		if (aTime < t0)
			return false;

		// set upper bound to index key
		i1 = aIndex;
		t1 = tt;
	}
	else
	{
		// set lower bound to index key
		i0 = aIndex;
		t0 = tt;

		// set upper bound to last key
		i1 = mCount-1;
		t1 = mKeys[i1 * mStride];
		if (aTime > t1)
			return false;
	}

	// while still checking a range of keys...
	while (i0 <= i1)
	{
		// estimate index
		float im = Lerp(float(i0), float(i1), (aTime - t0) / (t1 - t0));

		// if time is before segment start
		int iL = xs_FloorToInt(im);
		float tL = mKeys[iL * mStride];
		if (aTime < tL - FLT_EPSILON)
		{
			// set upper bound to segment start
			i1 = iL;
			t1 = tL;
			continue;
		}

		// if time is after segment end...
		int iH = xs_CeilToInt(im);
		float tH = mKeys[iH * mStride];
		if (aTime > tH + FLT_EPSILON)
		{
			// set lower bound to segment end
			i0 = iH;
			t0 = tH;
			continue;
		}

		// found!
		aIndex = iL;
		break;
	}

	// interpolate the value
	float *key0 = mKeys + aIndex * mStride;
	float *key1 = key0 + mStride;
	float t = (aTime - key0[0]) / (key1[0] - key0[0] + FLT_EPSILON);
	for (int element = 0; element < mWidth; element++)
	{
		aTarget[element] = Lerp(key0[1 + element], key1[1 + element], t);
	}
	return true;
}
