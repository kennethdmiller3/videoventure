#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"
#include "SoundStargate.h"

using namespace SoundStargate;

static bool Configure(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id)
{
	// clock frequency
	int frequency = 0;
	element->QueryIntAttribute("frequency", &frequency);

	// frequency divider
	float divider = 1;
	element->QueryFloatAttribute("divider", &divider);

	// clock tick counter
	unsigned int prevticks = 0;
	unsigned int nextticks = 0;

	// sample counter
	float samplespertick = float(AUDIO_FREQUENCY*divider)/float(frequency);
	float samples = 0;

	/*
	// sound 0x15	; enemy warp in 
	unsigned char delaydelta = 0xFE;	// [0x1A]
	unsigned char delay = 0xC0;			// A
	unsigned char repeat = 0x10;		// B
	*/

	int outersteps = 0;
	element->QueryIntAttribute("outersteps", &outersteps);

	int outerdelay = 0;
	element->QueryIntAttribute("outerdelay", &outerdelay);

	int innersteps = 0;
	element->QueryIntAttribute("innersteps", &innersteps);

	int innerdelay = 0;
	element->QueryIntAttribute("innerdelay", &innerdelay);

	int innerdelta = 0;
	element->QueryIntAttribute("innerdelta", &innerdelta);

	int amplitude = 255;
	element->QueryIntAttribute("amplitude", &amplitude);

	int amplitudedelta = 0;
	element->QueryIntAttribute("amplitudedelta", &amplitudedelta);

	// sound generator loop F89E
	unsigned char output = 0xFF;

	// pre-compute size
	int count = xs_FloorToInt(outersteps * (innersteps * (innerdelay + innerdelta * (outersteps - 1) / 2) + outerdelay) * samplespertick);
	self.Reserve(count);

	// for each outer step...
	for (int outer = outersteps; outer > 0; --outer)
	{
		// for each inner step
		for (int inner = innersteps; inner > 0; --inner)
		{
			// if a bit shifts out...
			if (random & 1)
			{
				// output samples
				samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
				prevticks = nextticks;

				// flip output
				output = output ? 0 : unsigned char(amplitude);
			}

			// update shift register
			random = ((((random >> 3) ^ (random)) & 1) << 15) | (random >> 1);

			// update tick count
			nextticks += innerdelay;
		}

		// update tick count
		nextticks += outerdelay;

		// update delay
		innerdelay += innerdelta;

		// update amplitude
		amplitude += amplitudedelta;
	}

	// output samples
	samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);

	return true;
}

static SoundConfigure::Auto soundbitnoiseramp(0xcc6424ae /* "bitnoiseramp" */, Configure);
