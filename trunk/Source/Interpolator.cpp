#include "StdAfx.h"
#include "Interpolator.h"

namespace Database
{
	Typed<Typed<InterpolatorTemplate> > interpolatortemplate(0x9074abfe /* "interpolatortemplate" */);
}

InterpolatorTemplate::InterpolatorTemplate(int aWidth)
	: mWidth(aWidth), mStride(2 + 2 * aWidth), mCount(0), mKeys(NULL)
{
}

InterpolatorTemplate::~InterpolatorTemplate()
{
	free(mKeys);
}

float *InterpolatorTemplate::AddKey(float aDuration)
{
	mKeys = static_cast<float *>(realloc(mKeys, (mCount + 1) * mStride * sizeof(float)));
	memset(mKeys + mCount * mStride, 0, mStride * sizeof(float));
	if (mCount > 0)
		mKeys[mCount * mStride] = mKeys[(mCount - 1)* mStride + 1];
	mKeys[mCount * mStride + 1] = mKeys[mCount * mStride] + aDuration;
	float *key = mKeys + mCount * mStride;
	++mCount;
	return key;
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
	while (aTime > mKeys[aIndex * mStride + 1])
	{
		// quit if out of range
		if (aIndex >= mCount - 1)
			return false;

		// go to the next index
		++aIndex;
	}

	// interpolate the value
	float *key = mKeys + aIndex * mStride;
	float t = aTime - key[0];
	for (int element = 0; element < mWidth; element++)
	{
		aTarget[element] = key[2 + 2 * element] + t * key[2 + 2 * element + 1];
	}
	return true;
}

static void ProcessInterpolatorKeyItem(TiXmlElement *element, InterpolatorTemplate &interpolator, const char *names[], const float data[])
{
	// get key duration
	float duration = FLT_MAX;
	element->QueryFloatAttribute("time", &duration);

	// add a key
	float *key = interpolator.AddKey(duration);

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x652b04df /* "start" */:
			for (int i = 0; i < interpolator.mWidth; i++)
			{
				key[2+i*2] = data[i];
				child->QueryFloatAttribute(names[i], &key[2+i*2]);
			}
			break;

		case 0x6a8e75aa /* "end" */:
			for (int i = 0; i < interpolator.mWidth; i++)
			{
				key[2+i*2+1] = key[2+i*2];
				child->QueryFloatAttribute(names[i], &key[2+i*2+1]);
			}
			break;
		}
	}

	// convert to velocity
	for (int i = 0; i < interpolator.mWidth; i++)
	{
		key[2+i*2+1] = (key[2+i*2+1] - key[2+i*2]) / (key[1] - key[0]);
	}
}

void ProcessInterpolatorItem(TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char *names[], const float data[])
{
	if (!element->FirstChildElement())
		return;

	// temporary interpolator
	InterpolatorTemplate interpolator(width);

	// start at zero time
	float time = 0.0f;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xd00594c0 /* "linear" */:
			{
				// process key data
				ProcessInterpolatorKeyItem(child, interpolator, names, data);
			}
			break;
		}
	}

	// push into the buffer
	buffer.push_back(interpolator.mCount);
	for (int i = 0; i < interpolator.mCount * interpolator.mStride; i++)
		buffer.push_back(*reinterpret_cast<unsigned int *>(&interpolator.mKeys[i]));
}
