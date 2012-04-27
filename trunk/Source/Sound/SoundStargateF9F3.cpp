#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"

// humanoid falling
static bool Configure(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id)
{
	// clock frequency
	int frequency = 3579000;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 4;
	element->QueryFloatAttribute("divider", &divider);

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;


// jump table entry 0x0C (sound 0x1A)
	unsigned char A, B;
	unsigned short X;
	unsigned char output = 0;
	unsigned char mem_0x11 = 0;
	unsigned char mem_0x12 = 0;
	unsigned char mem_0x13[10] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned short length = 8;

	do
	{
		do
		{
			X = 0;
			mem_0x11 = 0x80;
			B = 0;
			nextticks += 3+2+4+2;
			do
			{
				mem_0x13[X+1] += mem_0x13[X+0];
				if (char(mem_0x13[X+1]) < 0)
					B += mem_0x11;
				mem_0x11 >>= 1;
				++X;
				++X;
			}
			while (X < length);

			nextticks += 4*(5+5+6+4+6+4+4+3+4);
			samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
			prevticks = nextticks;

			output = B;

			nextticks += 5+6+4;
		}
		while (++mem_0x12 != 0);

		X = 0;
		B = 0;
		nextticks += 5;
		do
		{
			A = mem_0x13[X+0];
			nextticks += 5+4;
			if (A != 0)
			{
				nextticks += 2+4;
				if (A == 0x37)
				{
					nextticks += 2+6;
					B = 0x41;
					mem_0x13[X+2] = B;
				}
				nextticks += 7+2;
				--mem_0x13[X+0];
				++B;
			}
			nextticks += 4+4+3+4;
			++X;
			++X;
		}
		while (X < length);

		nextticks += 2+4;
	}
	while (B != 0);

	samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
	prevticks = nextticks;

	return true;
}

SoundConfigure::Auto soundstargateF9F3(0x071661ac /* "stargateF9F3" */, Configure);
