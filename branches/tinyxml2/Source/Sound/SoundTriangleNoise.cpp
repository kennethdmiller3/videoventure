#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"
#include "SoundStargate.h"

using namespace SoundStargate;

static bool Configure(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id)
{
	// linear feedback shift register
	unsigned short baseslope = 0;		// [0x13,0x15]
	unsigned short duration = 0;		// [0x16,0x17]
	unsigned char decay = 0;			// [0x18]
	unsigned char sputter = 0;			// [0x19]
	unsigned char output = 0;			// [0x400]

	// clock frequency
	int frequency = 0;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 1;
	element->QueryFloatAttribute("divider", &divider);

	// outer loop delay ticks
	int outerdelay = xs_FloorToInt(frequency/(divider*AUDIO_FREQUENCY));
	element->QueryIntAttribute("outerdelay", &outerdelay);

	// inner loop delay ticks
	int innerdelay = xs_FloorToInt(frequency/(divider*AUDIO_FREQUENCY));
	element->QueryIntAttribute("innerdelay", &innerdelay);

	int value;

	// base slope
	if (element->QueryIntAttribute("slope", &value) == tinyxml2::XML_SUCCESS)
		baseslope = unsigned short(value << 8);

	// duration
	if (element->QueryIntAttribute("duration", &value) == tinyxml2::XML_SUCCESS)
		duration = unsigned short(value);

	// decay
	if (element->QueryIntAttribute("decay", &value) == tinyxml2::XML_SUCCESS)
		decay = value != 0;

	// sputter
	if (element->QueryIntAttribute("sputter", &value) == tinyxml2::XML_SUCCESS)
		sputter = value != 0;

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;

	/// sound generator loop F930
	do
	{
		// loop counter
		unsigned short X = duration;

		// current value 8:8
		unsigned short value = output << 8;
		do
		{
			// update linear feedback shift register
			// random = ((((random >> 3) ^ random) & 1) << 15) | (random >> 1);
			// not sure what difference using the value makes,
			// but that's what the original M6808 code does
			random = ((((value >> 11) ^ random) & 1) << 15) | (random >> 1);

			// get slope 8:8 for this iteration
			unsigned short curslope = baseslope;

			// if applying sputter...
			if (sputter)
			{
				// randomize the slope
				curslope &= random | 0xFF;
			}

			// target value 8:8
			unsigned short target = (random << 8) & 0xFFFF;

			// if the current value is less than or equal to the target value...
			if (value <= target)
			{
				// ramp up towards the target
				do
				{
					// count down duration
					--X;
					if (X == 0)
						goto F985;

					// output the current value
					output = value >> 8;
					nextticks += innerdelay;
					samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
					prevticks = nextticks;

					// get the new value
					unsigned int newvalue = unsigned int(value) + unsigned int(curslope);

					// stop if the value overshot and wrapped around
					if (newvalue > 0xFFFF)
						break;

					// update the current value
					value = newvalue & 0xFFFF;
				}
				while (value <= target);
			}
			else
			{
				// ramp down towards the target
				do
				{
					// count down duration
					--X;
					if (X == 0)
						goto F985;

					// output the current value
					output = value >> 8;
					nextticks += innerdelay;
					samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
					prevticks = nextticks;

					// get the new value
					unsigned int newvalue = unsigned int(value) - unsigned int(curslope);

					// stop if the value overshot and wrapped around
					if (newvalue > 0xFFFF)
						break;

					// update the current value
					value = newvalue & 0xFFFF;
				}
				while (value > target);
			}

			// snap to target
			value = target;

			// add ticks
			nextticks += outerdelay;

			// output the current value
			output = value >> 8;
			samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
			prevticks = nextticks;
		}
		while(true);

F985:
		// if applying decay
		if (decay)
		{
			// scale slope by 7/8
			baseslope -= (baseslope >> 3);
			nextticks += 4*2+2*8+4*3+5*2+4;
		}
		else
		{
			// mark as repeating
			self.mRepeat = true;
			break;
		}
	}
	while (baseslope > 0x0007);

	// ramp to zero to prevent popping
	while (output != 0)
	{
		--output;
		nextticks += 8*4*256/baseslope;
		samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
		prevticks = nextticks;
	}

	return true;
}

static SoundConfigure::Auto soundtrianglenoise(0x0a2a7b91 /* "trianglenoise" */, Configure);
