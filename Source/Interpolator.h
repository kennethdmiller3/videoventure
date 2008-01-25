#pragma once

class InterpolatorTemplate
{
public:
	int mWidth;		// data width in floats
	int mStride;	// data stride in floats
	int mCount;		// number of keyframes
	float *mKeys;	// keyframe data
	
public:
	InterpolatorTemplate(int aWidth = 0);
	~InterpolatorTemplate();

	float *GetKey(int aIndex)
	{
		return mKeys + aIndex * mStride;
	}
	float *GetValues(int aIndex)
	{
		return mKeys + aIndex * mStride + 1;
	}
	float *AddKey(float aTime);

	bool Apply(float aTarget[], float aTime, int &aIndex);
};

void ProcessInterpolatorItem(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char *names[], const float data[]);
