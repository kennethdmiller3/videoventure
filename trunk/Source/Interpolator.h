#pragma once

bool ConfigureInterpolatorItem(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float data[]);

bool ApplyInterpolatorConstant(float aTarget[], int aWidth, int aCount, const float aKeys[], float aTime, int &aIndex);
bool ApplyInterpolator(float target[], int width, int count, const float keys[], float aTime, int &aIndex);

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

	bool Apply(float aTarget[], float aTime, int &aIndex)
	{
		ApplyInterpolator(aTarget, mWidth, mCount, mKeys, aTime, aIndex);
	}
};
