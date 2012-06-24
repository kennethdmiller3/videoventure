#pragma once

class PerfTimer
{
public:
	static const int NUM_SAMPLES = 640;
	static LONGLONG mFrequency;
	static int mIndex;
	static int mCount;

public:
	static void Init();
	static void Next();

public:
	LONGLONG mHistory[NUM_SAMPLES];
	LONGLONG mStamp;

public:
	PerfTimer();

	void Clear();

	void Start();
	void Stop();

	void Stamp();

	LONGLONG Ticks()
	{
		return mHistory[mIndex];
	}

	int Microseconds()
	{
		return int(1000000 * mHistory[mIndex] / mFrequency);
	}
};
