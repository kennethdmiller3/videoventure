#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"

// 
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

	/*
	;;; sound parameters
	A=00 (sound 0x1C):	FD3C	40 01 00 10 E1 0080 FF FF	; ??? enforcer fire
	A=01 (sound 0x1D):	FD45	28 01 00 08 81 0200 FF FF	; extra life
	A=02 (sound 0x1E):	FD4E	28 81 00 FC 01 0200 FC FF	; startup
	A=03 (sound 0x1F):	FD57	FF 01 00 18 41 0480 00 FF	; ???

	;;; arguments
	; 0x13: first pulse loop count at start of middle wave loop
	; 0x14: second pulse loop count at start of middle wave loop
	; 0x15: first pulse loop delta per iteration of middle wave loop
	; 0x16: second pulse loop delta per iteration of middle wave loop
	; 0x17: end middle wave loop second pulse loop count equals this
	; 0x18,0x19: inner wave loop total length
	; 0x1A: first pulse loop delta per iteration of outer wave loop
	; 0x1B: starting output value

	;;; locals
	; 0x1C: first pulse loop counter
	; 0x1D: second pulse loop counter
	*/

	int value;

	unsigned char pulse1delay = 0;			// [0x13]
	if (element->QueryIntAttribute("pulse1delay", &value) == tinyxml2::XML_SUCCESS)
		pulse1delay = unsigned char(value);

	unsigned char pulse2delay = 0;			// [0x14]
	if (element->QueryIntAttribute("pulse2delay", &value) == tinyxml2::XML_SUCCESS)
		pulse2delay = unsigned char(value);

	unsigned char pulse1innerdelta = 0;		// [0x15]
	if (element->QueryIntAttribute("pulse1innerdelta", &value) == tinyxml2::XML_SUCCESS)
		pulse1innerdelta = unsigned char(value);

	unsigned char pulse2innerdelta = 0;		// [0x16]
	if (element->QueryIntAttribute("pulse2innerdelta", &value) == tinyxml2::XML_SUCCESS)
		pulse2innerdelta = unsigned char(value);

	unsigned char pulse1outerdelta = 0;		// [0x1A]
	if (element->QueryIntAttribute("pulse1outerdelta", &value) == tinyxml2::XML_SUCCESS)
		pulse1outerdelta = unsigned char(value);

	unsigned char pulse2outerdelta = 0;		// new!
	if (element->QueryIntAttribute("pulse2outerdelta", &value) == tinyxml2::XML_SUCCESS)
		pulse2outerdelta = unsigned char(value);

	unsigned char pulse1output = 0;				// ~[0x1B]
	if (element->QueryIntAttribute("pulse1output", &value) == tinyxml2::XML_SUCCESS)
		pulse1output = unsigned char(value);

	unsigned char pulse2output = 0;				// [0x1B]
	if (element->QueryIntAttribute("pulse2output", &value) == tinyxml2::XML_SUCCESS)
		pulse2output = unsigned char(value);

	int totalpulsedelay = 0;				// [0x18,0x19]
	element->QueryIntAttribute("totalpulsedelay", &totalpulsedelay);

	int innercount = 0;						// smallest value such that ([0x14] + [0x16] * innercount) % 256 == [0x17]
	element->QueryIntAttribute("innercount", &innercount);

	int outercount = 0;						// smallest value such that ([0x13] + [0x1A] * outercount) % 256 == 0
	element->QueryIntAttribute("outercount", &outercount);

	int outerdelaypre = 0;
	element->QueryIntAttribute("outerdelaypre", &outerdelaypre);

	int innerdelaypre = 0;
	element->QueryIntAttribute("innerdelaypre", &innerdelaypre);

	int pulsedelaybase = 0;
	element->QueryIntAttribute("pulsedelaybase", &pulsedelaybase);

	int pulsedelayscale = 1;
	element->QueryIntAttribute("pulsedelayscale", &pulsedelayscale);

	int innerdelaypost = 0;
	element->QueryIntAttribute("innerdelaypost", &innerdelaypost);

	int outerdelaypost =0;
	element->QueryIntAttribute("outerdelaypost", &outerdelaypost);


	// sound generator loop

	// outer repeat loop
	for (int outer = outercount; outer > 0; --outer)
	{
		unsigned char pulse1 = pulse1delay;
		unsigned char pulse2 = pulse2delay;

		nextticks += outerdelaypre;
		OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
		prevticks = nextticks;

		// inner repeat loop
		for (int inner = innercount; inner > 0; --inner)
		{
			int delaylimit = totalpulsedelay;

			nextticks += innerdelaypre;
			OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
			prevticks = nextticks;

			// pulse loop
			do	
			{
				int steps;
				
				// pulse 1
				steps = std::min<int>(pulse1, delaylimit);
				nextticks += pulsedelaybase + pulsedelayscale * steps;
				OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse1output);
				prevticks = nextticks;

				delaylimit -= steps;
				if (delaylimit == 0)
					break;

				// pulse 2
				steps = std::min<int>(pulse2, delaylimit);
				nextticks += pulsedelaybase + pulsedelayscale * steps;
				OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
				prevticks = nextticks;

				delaylimit -= steps;
				if (delaylimit == 0)
					break;
			}
			while (true);

			// inner delay
			nextticks += innerdelaypost;
			OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
			prevticks = nextticks;

			// update pulse delays
			pulse1 = (pulse1 + pulse1innerdelta) & 0xFF;
			pulse2 = (pulse2 + pulse2innerdelta) & 0xFF;
		}


		// outer delay;
		nextticks += outerdelaypost;
		OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * pulse2output);
		prevticks = nextticks;

		// update pulse delays
		pulse1delay = (pulse1delay + pulse1outerdelta) & 0xFF;
		pulse2delay = (pulse2delay + pulse2outerdelta) & 0xFF;
	}

	return true;
}

static SoundConfigure::Auto soundpulseloop(0x36f16844 /* "pulseloop" */, Configure);
