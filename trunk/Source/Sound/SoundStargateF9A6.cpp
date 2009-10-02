#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"

// "Big Red" fire sound
static bool Configure(SoundTemplate &self, const TiXmlElement *element, unsigned int id)
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

	// wave data
	unsigned char data[16] =
	{
		0x8C, 0x5B, 0xB6, 0x40, 0xBF, 0x49, 0xA4, 0x73,
		0x73, 0xA4, 0x49, 0xBF, 0x40, 0xB6, 0x5B, 0x8C,
	};

	unsigned char mem_0x11 = 0x00;
	unsigned short mem_0x0B_0x0C = 0x0064;
	unsigned char A = 0xFD, B = 0x00;
	unsigned short X;

	do
	{
		A = mem_0x11;
		int value = (A << 8) + B + mem_0x0B_0x0C;
		A = (value >> 8) & 0xFF;
		B = value & 0xFF;
		mem_0x11 = A;
		X = mem_0x0B_0x0C;
		if (value > 0xFFFF)
		{
			++X;
			nextticks += 4+4;
			if (X == 0x0070)
				break;
		}
		mem_0x0B_0x0C = X;
		nextticks += 4+4+4+5+4+4+4+5+4+4+5+5+4+5+4;
		OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * data[A & 0x0F]);
		prevticks = nextticks;
	}
	while(true);

	return true;
}

static SoundConfigure::Auto stargateF9A6(0x920eedb8 /* "stargateF9A6" */, Configure);
