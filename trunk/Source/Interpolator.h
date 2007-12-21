#pragma once

class InterpolatorTemplate
{
public:
	int mWidth;
	int mStride;
	int mCount;
	float *mKeys;

public:
	InterpolatorTemplate(int aWidth = 0);
	~InterpolatorTemplate();

	float *AddKey(float aDuration);

	bool Apply(float aTarget[], float aTime, int &aIndex);
};

struct Interpolator
{
public:
	InterpolatorTemplate *mTemplate;

public:
};

void ProcessInterpolatorItem(TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char *names[], const float data[]);
