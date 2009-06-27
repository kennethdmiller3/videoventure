#include "StdAfx.h"
#include "PerfTimer.h"


LONGLONG PerfTimer::mFrequency;
int PerfTimer::mIndex;
int PerfTimer::mCount;


void PerfTimer::Init()
{
	// get frequency
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	mFrequency = freq.QuadPart;

	// reset counters
	mIndex = NUM_SAMPLES - 1;
	mCount = 0;
}

void PerfTimer::Next()
{
	if (++mIndex >= NUM_SAMPLES)
		mIndex = 0;
	if (++mCount > NUM_SAMPLES)
		mCount = NUM_SAMPLES;
}

PerfTimer::PerfTimer()
{
	// clear history
	memset(mHistory, 0, sizeof(mHistory));
}

void PerfTimer::Clear()
{
	mHistory[mIndex] = 0;
}

void PerfTimer::Start()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	mStamp = count.QuadPart;
}

void PerfTimer::Stop()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	mHistory[mIndex] += count.QuadPart - mStamp;
}

void PerfTimer::Stamp()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	mHistory[mIndex] = count.QuadPart - mStamp;
	mStamp = count.QuadPart;
}
