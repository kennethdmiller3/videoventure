#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"

static size_t ReadBinaryData(const char *data, unsigned char buffer[], size_t size)
{
	size_t count = 0;
	size_t len = strlen(data);
	bool high = false;
	for (size_t i = 0; i < len; ++i)
	{
		unsigned char value;
		if (data[i] >= '0' && data[i] <= '9')
			value = unsigned char(data[i] - '0');
		else if (data[i] >= 'A' && data[i] <= 'F')
			value = unsigned char(data[i] - 'A' + 10);
		else if (data[i] >= 'a' && data[i] <= 'f')
			value = unsigned char(data[i] - 'a' + 10);
		else
		{
			high = false; continue;
		}

		if (high)
		{
			buffer[count] = (buffer[count] << 4) + value;
			++count;
			high = false;
		}
		else
		{
			buffer[count] = value;
			high = true;
		}
		if (count >= size)
			break;
	}
	return count;
}

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


	/*
	;;; parameter table
	A=00 (sound 0x01):	FEEA	81 24 00 00 00 16 31	; enemy kill
	A=01 (sound 0x02):	FEF1	12 05 1A FF 00 27 6D	; space guppy fire
	A=02 (sound 0x03):	FEF8	11 05 11 01 0F 01 47	; lander fire
	A=03 (sound 0x04):	FEFF	11 31 00 01 00 0D 1B	; ???
	A=04 (sound 0x05):	FF06	F4 12 00 00 00 14 47	; pod explosion
	A=05 (sound 0x06):	FF0D	41 45 00 00 00 0F 5B	; lander kill
	A=06 (sound 0x07):	FF14	21 35 11 FF 00 0D 1B	; space guppy kill
	A=07 (sound 0x08):	FF1B	15 00 00 FD 00 01 69	; humanoid catch
	A=08 (sound 0x09):	FF22	31 11 00 01 00 03 6A	; firebomber/mutant fire
	A=09 (sound 0x0A):	FF29	01 15 01 01 01 01 47	; game start
	A=0A (sound 0x0B):	FF30	F6 53 03 00 02 06 94	; humanoid abduction
	A=0B (sound 0x0C):	FF37	6A 10 02 00 02 06 9A	; swarmer fire
	A=0C (sound 0x0D):	FF3E	1F 12 00 FF 10 04 69	; humanoid deposit, also used by sound 0x12


	; packed wave data table
	; variable-sized records: size | data[0] .. data[size-1]
	I=0:	FE4B	08 | 7F D9 FF D9 7F 24 00 24
	I=1:	FE54	08 | 00 40 80 00 FF 00 80 40
	I=2:	FE5D	10 | 7F B0 D9 F5 FF F5 D9 B0 7F 4E 24 09 00 09 24 4E
	I=3:	FE6E	10 | 7F C5 EC E7 BF 8D 6D 6A 7F 94 92 71 40 17 12 39
	I=4:	FE7F	10 | FF FF FF FF 00 00 00 00 FF FF FF FF 00 00 00 00
	I=5:	FE90	48 | 8A 95 A0 AB B5 BF C8 D1 DA E1 E8 EE F3 F7 FB FD
						 FE FF FE FD FB F7 F3 EE E8 E1 DA D1 C8 BF B5 AB
						 A0 95 8A 7F 75 6A 5F 54 4A 40 37 2E 25 1E 17 11
						 0C 08 04 02 01 00 01 02 04 08 0C 11 17 1E 25 2E
						 37 40 4A 54 5F 6A 75 7F
	I=6:	FED9	10 | 59 7B 98 AC B3 AC 98 7B 59 37 19 06 00 06 19 37


	; packed period data table
	+1B:	FF6E	0D | 01 01 02 02 03 04 05 06 07 08 09 0A
	+31:	FF84	16 | 01 01 02 02 04 04 08 08 10 20 28 30 38 40 48 50
						 60 70 80 A0 B0 C0
	+47:	FF9A	14 | 08 40 08 40 08 40 08 40 08 40 08 40 08 40 08 40
						 08 40 08 40
	+5B:	FFAE	0F | 01 02 04 08 09 0A 0B 0C 0E 0F 10 12 14 16 40
	+69:	FFBC	04 | 40 10 08 01
	+6A:	FFBD	03 | 10 08 01
	+6D:	FFC0	27 | 01 01 01 01 02 02 03 03 04 04 05 06 08 0A 0C 10
						 14 18 20 30 40 50 40 30 20 10 0C 0A 08 07 06 05
						 04 03 02 02 01 01 01
	+94:	FFE7	06 | 07 08 09 0A 0C 08
	+9A:	FFED	06 | 17 18 19 1A 1B 1C


	;;; input values
	[0x07]: set by IRQ handler to [0x04]|[0x05]

	;;; unpacked parameters
	[0x13]: high nybble of record[0]		(period table repeat count)
	[0x14]: low nybble of record[0]			(wave table repeat count)
	[0x15]: high nybble of record[1]		(fade delta per period table repeat)
	[0x16]: record[3]						(period offset delta)
	[0x17]: record[4]						(outer loop count)
	[0x18,0x19]: address of wave data entry indexed by low nybble of record[1]
	[0x1A]: record[2]						(fade start)
	[0x1B,0x1C]: 0xFF53 + record[6]			(start of period data)
	[0x1D,0x1E]: 0xFF53 + record[6] + record[5]	(end of period data)
	[0x24...]: copied wave data

	;;; local values
	[0x0D,0x0E] stored period data pointer
	[0x0F,0x10] stored wave data pointer
	[0x1F,0x20] end of copied wave data
	[0x21]: current period value
	[0x22]: current period repeat counter
	[0x23]: period add
	*/

	unsigned char B;
	unsigned char output = 0;

#if 1
	unsigned int wavelength = 0;
	unsigned char wavesource[0x80] = { 0 };

	unsigned int pitchlength = 0;
	unsigned char pitchsource[0x40] = { 0 };

	int pitchrepeat = 0;		// [0x13], high nybble of record[0]
	element->QueryIntAttribute("delayrepeat", &pitchrepeat);

	int waverepeat = 0;			// [0x14], low nybble of record[0]
	element->QueryIntAttribute("waverepeat", &waverepeat);
		
	int fadedelta = 0;			// [0x15], high nybble of record[1]
	element->QueryIntAttribute("fadedelta", &fadedelta);

	int fadestart = 0;			// [0x1A], record[2]
	element->QueryIntAttribute("fadestart", &fadestart);

	int pitchdelta = 0;			// [0x16], record[3]
	element->QueryIntAttribute("delaydelta", &pitchdelta);

	int outerrepeat = 0;		// [0x17], record[4]
	element->QueryIntAttribute("outerrepeat", &outerrepeat);

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch(Hash(child->Value()))
		{
		case 0xa9f017d4 /* "wave" */:
			{
				if (const char *data = child->Attribute("data"))
					wavelength = ReadBinaryData(data, wavesource, SDL_arraysize(wavesource));

				for (const TiXmlElement *data = child->FirstChildElement(); data != NULL; data = data->NextSiblingElement())
				{
					switch(Hash(data->Value()))
					{
					case 0xd872e2a5 /* "data" */:
						{
							int value = 0;
							data->QueryIntAttribute("value", &value);
							wavesource[wavelength++] = unsigned char(value);
						}
						break;
					}
				}
			}
			break;

		case 0x4ed1f1d8 /* "delay" */:
			{
				if (const char *data = child->Attribute("data"))
					pitchlength = ReadBinaryData(data, pitchsource, SDL_arraysize(pitchsource));

				for (const TiXmlElement *data = child->FirstChildElement(); data != NULL; data = data->NextSiblingElement())
				{
					switch(Hash(data->Value()))
					{
					case 0xd872e2a5 /* "data" */:
						{
							int value = 0;
							data->QueryIntAttribute("value", &value);
							pitchsource[pitchlength++] = unsigned char(value);
						}
						break;
					}
				}
			}
			break;
		}
	}

#endif

	unsigned char *pitchbufferstart = pitchsource, *pitchbufferend = pitchsource + pitchlength;

	// clear delay offset
	int pitchoffset = 0;

	// sound generator loop
	do
	{
		int fade = fadestart;

		// for each iteration of the pitch loop
		for (int pitchloop = pitchrepeat; pitchloop != 0; --pitchloop)
		{
			nextticks += 3+5;

			// for each entry in the delay table...
			for (unsigned char *pitchptr = pitchbufferstart; pitchptr != pitchbufferend; ++pitchptr)
			{
				// compute pitch value
				int pitch = *pitchptr + pitchoffset;

				nextticks += 4+5+3+4+4+4+3+4+5;

				// set repeat counter
				for (int waveloop = waverepeat; waveloop != 0; --waveloop)
				{
					// output one wave pattern
					nextticks += 3;
					for (unsigned char *waveptr = wavesource; waveptr != wavesource + wavelength; ++waveptr)
					{
						nextticks += 3+(2+4)*pitch+5;

						samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
						prevticks = nextticks;

						output = unsigned char(*waveptr - fade * (*waveptr >> 4));

						nextticks += 5+4+4+4;
					}

					nextticks += 2+4;
					nextticks += 6*4+2*2+4;
				}

				nextticks -= 6*4+2*2+4;	// cancel delay from last iteration

				// TO DO: compute cycle count
				samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
				prevticks = nextticks;
			}

			// fade copied wave by fadedelta
			fade += fadedelta;
			nextticks += 3+5+6+4;
			nextticks += fadedelta ? 27+wavelength*(65+fadedelta*12) : 11;
		}

		// get delay offset delta
		if (pitchdelta == 0)
			break;

		// decrement repeat counter
		--outerrepeat;
		if (outerrepeat == 0)
			break;

		// TO DO: compute cycle count

		// apply delta to delay offset value
		pitchoffset += pitchdelta;

		// adjust delay table
		unsigned char *pitchptr = pitchbufferstart;

		B = 0;
		nextticks += 4+4+4+4+4+4+3+4+4+2;
		nextticks -= (3+6+4+5+4+4+2+4+4+4)*(pitchptr-pitchsource);
		do
		{
			// get pitch table entry plus pitch offset
			unsigned int pitch = unsigned char(pitchoffset) + *pitchptr;

			// if the pitch offset delta is positive...
			if (pitchdelta >= 0)
			{
				// if the combined pitch wraps around...
				if (pitch > 0xFF)
					goto FC27;
				goto FC2C;
			}

			// if the combined pitch reaches zero...
			if ((pitch & 0xFF) == 0)
				goto FC27;
			if (pitch > 0xFF)
				goto FC2C;
FC27:
			if (B == 0)
				continue;

			// set end of table
			pitchbufferend = pitchptr;
			break;
FC2C:
			if (B != 0)
				continue;

			// set beginning of table
			pitchbufferstart = pitchptr;
			++B;
		}
		while (++pitchptr < pitchbufferend);
		nextticks += (3+6+4+5+4+4+2+4+4+4)*(pitchptr-pitchsource);

		if (B == 0)
			break;

		nextticks += 2+4;

		// if applying fade...
		if (fadedelta != 0)
		{
			// reinitialize wave data
			nextticks += 5+57+43*wavelength;
			nextticks += 3;
			nextticks += fadestart ? 5+27+wavelength*(65+fadestart*12) : 5+11;
		}
		nextticks += 5;
	}
	while (1);

	samples = OutputPulse(self, nextticks - prevticks, samplespertick, samples, 128 * output);
	prevticks = nextticks;
	return true;
}

static SoundConfigure::Auto soundwaveloop(0xde15262a /* "waveloop" */, Configure);
