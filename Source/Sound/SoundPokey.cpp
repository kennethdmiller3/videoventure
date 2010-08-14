#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"

#define SOUND_POKEY_USE_EXPRESSION
#ifdef SOUND_POKEY_USE_EXPRESSION
#include "ExpressionConfigure.h"
#else
#include "Interpolator.h"
#endif

#define POKEY_TYPE_MAME 0
#define POKEY_TYPE_ATARI800 1
#define POKEY_TYPE_GALOIS 2

#define POKEY_TYPE POKEY_TYPE_ATARI800


static class SoundPokey
{
public:
	bool poly4[(1<<4) - 1];
	bool poly5[(1<<5) - 1];
	bool poly9[(1<<9) - 1];
	bool poly17[(1<<17) - 1];

public:
	SoundPokey()
	{
		SoundConfigure::Add(0xe8f2b85f /* "pokey" */, SoundConfigure::Entry(this, &SoundPokey::Configure));

#if POKEY_TYPE == POKEY_TYPE_MAME

		InitPoly(poly4,   4,  3, 1, 0x00004);
		//Poly4:
		//000111101011001|15

		InitPoly(poly5,   5,  3, 2, 0x00008);
		//Poly5:
		//0000011100100010101111011010011|31

		InitPoly(poly9,   9,  8, 1, 0x00180);
		//Poly9:
		//0000000010101010110011001110111011110000111110101111110011111110111111110|73
		//0000000010101010110011001110111011110000111110101111110011111110111111110|146
		//0000000010101010110011001110111011110000111110101111110011111110111111110|219
		//0000000010101010110011001110111011110000111110101111110011111110111111110|292
		//0000000010101010110011001110111011110000111110101111110011111110111111110|365
		//0000000010101010110011001110111011110000111110101111110011111110111111110|438
		//0000000010101010110011001110111011110000111110101111110011111110111111110|511

		InitPoly(poly17, 17, 16, 1, 0x1C000);
		//Poly17:
		//00000000000000010010010010010010100010100010100011001101001101001110000110000110000010011100101100100010111000111000100011110011
		//11001100111110000001011000000010010001110010010100010000010100011001100100011001110000101001110000010011010000010010100001100100
		//01010100101100010001101000111001100111010011110000101110100000010011011101001001100001111010001011001011110100110110001111101000
		//01110011111101001011100000000110001111001001000010000001010001001100100110100110000101000011000010011010010110010100001100011100
		//01100101100111100111000111000000101110011110010011011100000010100001111001001101001011110001011010001111100110110100111111000011
		//10100000000100111101001001000101111010001010011011110100110100001111101000011001011111010010110001111110100011100111111101001111
		//00000000011000000100100100001001000101000100110001000110011000010001000010110010100110010101100011000010101011001110010101010110
		//00001010101010110010001101010101100010000110101011001100101101010110000101011010101100101010110101011000110101101010110011101011|33383|66766|100149

#elif POKEY_TYPE == POKEY_TYPE_ATARI800

		InitPoly(poly4, 4, 1, 0xF, false);
		//Poly4 tap=1 seed=0000000f invert=0
		//111100010011010|15

		InitPoly(poly5, 5, 2, 0x1F, true);
		//Poly5 tap=2 seed=0000001f invert=1
		//0000011100100010101111011010011|31

		InitPoly(poly9, 9, 5, 0x1FF, false);
		//Poly9 tap=5 seed=000001ff invert=0
		//11111111100001111011100001011001101101111010000111001100001001000101011101011110010010111001110000001110111010011110101001010000
		//00101010101111101011010000011011101101101011000001011101111100011110011010011010111000110100010111111101001011000101001100011000
		//00001100110010101100100111111011010010010011011111100101101010000101000100111011001011110110000110101010011100100001100010000100
		//0000001000100011001000111010101101100011100010010101000110110011111001111000101101110010100100000100110011101000111110111100000|511

		InitPoly(poly17, 17, 5, 0x1FFFF, false);
		//Poly17 tap=5 seed=0001ffff invert=0
		//11111111111111111000000000000111110000000111111111100111110000011000111111111000001110000111111110011011110001100111000110000100
		//00100000100000000001100001000001100010000011100001000111100110000101101001001001100010011011110001011111000110111101111000110000
		//00001100000110001100011110000000000110111100000110110001111110111100011101000001100101111011100110110000010001111101110001011110
		//00101011110011011010100100111100010001110101101001010110000010000100101110000000100111011100100110010010011111011110111011100000
		//00011001011100011001101110100001011101111100011111000111100111111001101000110110011011100111110111010000111000111110011001001111
		//10010111110111101011011000000100001110110100000110101001101110100100111101111001110100000010000111101010000011100101010111100010
		//01110100110101100111011100001100010010110100001010010001100010000101000001000001001011000011001001010101001111000111100101011001
		//10101001001101100100011111111111010111000000101001101110101000111101110101101100001110000111010110011011011000110111101110000110|131071

#elif POKEY_TYPE == POKEY_TYPE_GALOIS

		InitPoly(poly4, 4, (1<<3)|(1<<2), 0xF, false);
		//LFSR4 mask=0000000c seed=0000000f invert=0
		//111000100110101|15

		InitPoly(poly5, 5, (1<<4)|(1<<2), 0x1F, true);
		//LFSR5 mask=00000014 seed=0000001f invert=1
		//0001110010001010111101101001100|31

		InitPoly(poly9, 9, (1<<8)|(1<<3), 0x1FF, false);
		//LFSR9 mask=00000108 seed=000001ff invert=0
		//11110000111101110000101100110110111101000011100110000100100010101110101111001001011100111000000111011101001111010100101000000101
		//01010111110101101000001101110110110101100000101110111110001111001101001101011100011010001011111110100101100010100110001100000001
		//10011001010110010011111101101001001001101111110010110101000010100010011101100101111011000011010101001110010000110001000010000000
		//0100010001100100011101010110110001110001001010100011011001111100111100010110111001010010000010011001110100011111011110000011111|511

		InitPoly(poly17, 17, (1<<16)|(1<<11), 0x1FFFF, false);
		//LFSR17 mask=00010800 seed=0001ffff invert=0
		//11111111111100000000000011111000000011111111110011111000001100011111111100000111000011111111001101111000110011100011000010000100
		//00010000000000110000100000110001000001110000100011110011000010110100100100110001001101111000101111100011011110111100011000000001
		//10000011000110001111000000000011011110000011011000111111011110001110100000110010111101110011011000001000111110111000101111000101
		//01111001101101010010011110001000111010110100101011000001000010010111000000010011101110010011001001001111101111011101110000000011
		//00101110001100110111010000101110111110001111100011110011111100110100011011001101110011111011101000011100011111001100100111110010
		//11111011110101101100000010000111011010000011010100110111010010011110111100111010000001000011110101000001110010101011110001001110
		//10011010110011101110000110001001011010000101001000110001000010100000100000100101100001100100101010100111100011110010101100110101
		//00100110110010001111111111101011100000010100110111010100011110111010110110000111000011101011001101101100011011110111000011000000|131071

#else
#error "Undefine POKEY type"
#endif
/*
		Polynomials for Maximum LFSR:
		http://homepage.mac.com/afj/taplist.html

		Stargate 16-bit LFSR
		seed: 0x3C00
		output = random & 1
		random = ((((random >> 3) ^ (random)) & 1) << 15) | (random >> 1);
		L=57337: 00000000001111000000000111011100000011110011110001110110110111111100000000100001110000010010111111001000010100011000101011011101
*/
	}

#if POKEY_TYPE == POKEY_TYPE_MAME
	// from MAME pokey.c
	size_t InitPoly(bool aOut[], int aSize, int aLeft, int aRight, int aAdd)
	{
		DebugPrint("Poly%d:", aSize);
		int mask = (1 << aSize) - 1;
		int x = 0;
		int repeat = 0;
		for (int i = 0; i < mask; ++i)
		{
			if (i < 1024)
			{
				if (((i-repeat)&127)==0)
					DebugPrint("\n");
				DebugPrint(x&1?"1":"0");
			}
			aOut[i] = (x & 1) != 0;
			x = ((x << aLeft) + (x >> aRight) + aAdd) & mask;
			if (x == 0)
			{
				DebugPrint("|%d", i+1);
				repeat = i+1;
			}
		}
		DebugPrint("\n");
		return mask;
	}
#elif POKEY_TYPE == POKEY_TYPE_ATARI800
	// from Atari800 pokey.c
	size_t InitPoly(bool aOut[], int aSize, int aTap, unsigned int aSeed, bool aInvert = false)
	{
		DebugPrint("Poly%d tap=%d seed=%08x invert=%d", aSize, aTap, aSeed, aInvert);
		unsigned int x = aSeed;
		unsigned int i = 0;
		do
		{
			aOut[i] = (x & 1) ^ aInvert;
			if (i < 1024)
			{
				if ((i&127)==0)
					DebugPrint("\n");
				DebugPrint(aOut[i]?"1":"0");
			}
			x = ((((x >> aTap) ^ x) & 1) << (aSize - 1)) | (x >> 1);
			++i;
		}
		while (x != aSeed);
		DebugPrint("|%d\n", i);
		return i;
	}
#elif POKEY_TYPE == POKEY_TYPE_GALOIS
	// from http://en.wikipedia.org/wiki/Linear_feedback_shift_register
	size_t InitPoly(bool aOut[], int aSize, unsigned int aMask, unsigned int aSeed = 0, bool aInvert = false)
	{
		DebugPrint("LFSR%d mask=%08x seed=%08x invert=%d", aSize, aMask, aSeed, aInvert);
		unsigned int x = aSeed;
		unsigned int i = 0;
		do
		{
			aOut[i] = (x & 1) ^ aInvert;
			if (i < 1024)
			{
				if ((i&127)==0)
					DebugPrint("\n");
				DebugPrint(aOut[i]?"1":"0");
			}
			x = (x>>1) ^ (-int(x&1)&aMask);
			++i;
		}
		while (x != aSeed);
		DebugPrint("|%d\n", i);
		return i;
	}
#else
#error "Undefined POKEY type"
#endif

bool Configure(SoundTemplate &self, const TiXmlElement *element, unsigned int id)
{
	// sample length
	float length = 0;
	element->QueryFloatAttribute("length", &length);
	int samples = xs_CeilToInt(length * AUDIO_FREQUENCY);

	// reserve space
	self.Reserve(samples);

	// clock frequency
	int frequency = 0;
	element->QueryIntAttribute("frequency", &frequency);

#ifdef SOUND_POKEY_USE_EXPRESSION
	// expression stream
	std::vector<unsigned int> stream;

	// element names
	const char *names[1] = { "value" };

	// frequency divider
	float dividerdefault = 1.0f;
	float dividerquant = 1;
	element->QueryFloatAttribute("divider", &dividerdefault);
	if (const TiXmlElement *child = element->FirstChildElement("divider"))
	{
		Expression::Loader<float>::ConfigureRoot(child, stream, names, &dividerdefault);
		child->QueryFloatAttribute("quantize", &dividerquant);
	}
	else
	{
		Expression::Append(stream, Expression::Constant<float>, dividerdefault);
	}

	// amplitude
	float amplitudedefault = 1.0f;
	float amplitudequant = 1.0f/65536.0f;
	element->QueryFloatAttribute("amplitude", &amplitudedefault);
	if (const TiXmlElement *child = element->FirstChildElement("amplitude"))
	{
		Expression::Loader<float>::ConfigureRoot(child, stream, names, &amplitudedefault);
		child->QueryFloatAttribute("quantize", &amplitudequant);
	}
	else
	{
		Expression::Append(stream, Expression::Constant<float>, amplitudedefault);
	}

	// offset
	float offsetdefault = 0.0f;
	float offsetquant = 1.0f/65536.0f;
	element->QueryFloatAttribute("offset", &offsetdefault);
	if (const TiXmlElement *child = element->FirstChildElement("offset"))
	{
		Expression::Loader<float>::ConfigureRoot(child, stream, names, &offsetdefault);
		child->QueryFloatAttribute("quantize", &offsetquant);
	}
	else
	{
		Expression::Append(stream, Expression::Constant<float>, offsetdefault);
	}
#else
	// frequency divider
	float divider = 1;
	element->QueryFloatAttribute("divider", &divider);

	// amplitude
	float amplitude = 1.0f;
	element->QueryFloatAttribute("amplitude", &amplitude);

	// offset
	float offset = 0.0f;
	element->QueryFloatAttribute("offset", &offset);

	// keyframe data
	std::vector<unsigned int> dividerkey;
	float dividerquant = 1;
	std::vector<unsigned int> amplitudekey;
	float amplitudequant = 1.0f/65536.0f;
	std::vector<unsigned int> offsetkey;
	float offsetquant = 1.0f/65536.0f;

	const char *names[1] = { "value" };

	ApplyInterpolatorFunc dividerfunc = ApplyConstant;
	ApplyInterpolatorFunc amplitudefunc = ApplyConstant;
	ApplyInterpolatorFunc offsetfunc = ApplyConstant;

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x36b04926 /* "divider" */:
			{
				child->QueryFloatAttribute("value", &divider);
				child->QueryFloatAttribute("quantize", &dividerquant);
				if (ConfigureInterpolatorItem(child, dividerkey, 1, names, &divider))
				{
					int interpolate = 1;
					child->QueryIntAttribute("interpolate", &interpolate);
					dividerfunc = interpolate ? ApplyInterpolator : ApplyInterpolatorConstant;
				}
			}
			break;

		case 0x16746aa2 /* "amplitude" */:
			{
				child->QueryFloatAttribute("value", &amplitude);
				child->QueryFloatAttribute("quantize", &amplitudequant);
				int interpolate = 1;
				child->QueryIntAttribute("interpolate", &interpolate);
				amplitudefunc = interpolate ? ApplyInterpolator : ApplyInterpolatorConstant;
				ConfigureInterpolatorItem(child, amplitudekey, 1, names, &amplitude);
			}
			break;

		case 0x14c8d3ca /* "offset" */:
			{
				child->QueryFloatAttribute("value", &offset);
				child->QueryFloatAttribute("quantize", &offsetquant);
				int interpolate = 1;
				child->QueryIntAttribute("interpolate", &interpolate);
				offsetfunc = interpolate ? ApplyInterpolator : ApplyInterpolatorConstant;
				ConfigureInterpolatorItem(child, offsetkey, 1, names, &offset);
			}
			break;

		}
	}

	if (dividerkey.empty())
	{
		dividerkey.reserve(3);
		dividerkey.push_back(1);
		dividerkey.push_back(0);
		dividerkey.push_back(*reinterpret_cast<unsigned int *>(&divider));
		dividerfunc = ApplyConstant;
	}
	if (amplitudekey.empty())
	{
		amplitudekey.reserve(3);
		amplitudekey.push_back(1);
		amplitudekey.push_back(0);
		amplitudekey.push_back(*reinterpret_cast<unsigned int *>(&amplitude));
		amplitudefunc = ApplyConstant;
	}
	if (offsetkey.empty())
	{
		offsetkey.reserve(3);
		offsetkey.push_back(1);
		offsetkey.push_back(0);
		offsetkey.push_back(*reinterpret_cast<unsigned int *>(&offset));
		offsetfunc = ApplyConstant;
	}
#endif

	bool *poly1data = NULL;
	int poly1size = 1;
	bool *poly2data = NULL;
	int poly2size = 1;

	switch (Hash(element->Attribute("tone")))
	{
	case 0x966dd8e3 /* "pure" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = NULL;
		poly2size = 1;
		break;

	case 0x90ecc009 /* "poly4" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = poly4;
		poly2size = SDL_arraysize(poly4);
		break;

	case 0x9becd15a /* "poly9" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = poly9;
		poly2size = SDL_arraysize(poly9);
		break;

	case 0xd2ba0daf /* "poly17" */:
		poly1data = NULL;
		poly1size = 1;
		poly2data = poly17;
		poly2size = SDL_arraysize(poly17);
		break;

	case 0x8fecbe76 /* "poly5" */:
		poly1data = poly5;
		poly1size = SDL_arraysize(poly5);
		poly2data = NULL;
		poly2size = 1;
		break;

	case 0x3746e7fb /* "poly5+poly4" */:
		poly1data = poly5;
		poly1size = SDL_arraysize(poly5);
		poly2data = poly4;
		poly2size = SDL_arraysize(poly4);
		break;

	case 0x3246e01c /* "poly5+poly9" */:
		poly1data = poly5;
		poly1size = SDL_arraysize(poly5);
		poly2data = poly9;
		poly2size = SDL_arraysize(poly9);
		break;

	case 0x40a65239 /* "poly5+poly17" */:
		poly1data = poly5;
		poly1size = SDL_arraysize(poly5);
		poly2data = poly17;
		poly2size = SDL_arraysize(poly17);
		break;

	default:
		break;
	}

	const float sampleticks = float(frequency) / float(AUDIO_FREQUENCY);
	const float ticksamples = float(AUDIO_FREQUENCY) / float(frequency);
	const int startindex = xs_FloorToInt(self.mLength * ticksamples);

	float counter = 0;
	int poly1index = startindex % poly1size;
	int poly2index = startindex % poly2size;
	bool outputhigh = true;

#ifdef SOUND_POKEY_USE_EXPRESSION
	EntityContext context(&stream[0], stream.size(), 0, id);
#else
	float time = 0.0f;
	const float steptime = 1.0f / float(AUDIO_FREQUENCY);
	int dividerhint = 0;
	int amplitudehint = 0;
	int offsethint = 0;
#endif

	// for each sample...
	for (int i = 0; i < samples; ++i)
	{
#ifdef SOUND_POKEY_USE_EXPRESSION
		// set up context
		context.Restart();
		context.mParam = i / float(AUDIO_FREQUENCY);

		// get current divider value
		float divider = Expression::Evaluate<float>(context);
		divider = xs_RoundToInt(divider / dividerquant) * dividerquant;
		_ASSERTE(divider > 0);

		// get current amplitude value
		float amplitude = Expression::Evaluate<float>(context);
		amplitude = xs_RoundToInt(amplitude / amplitudequant) * amplitudequant;

		// get current offset value
		float offset = Expression::Evaluate<float>(context);
		offset = xs_RoundToInt(offset / offsetquant) * offsetquant;
#else
		// get current divider value
		float divider;
		dividerfunc(&divider, 1, dividerkey[0], reinterpret_cast<const float * __restrict>(&dividerkey[1]), time, dividerhint);
		divider = xs_RoundToInt(divider / dividerquant) * dividerquant;

		// get current amplitude value
		float amplitude;
		amplitudefunc(&amplitude, 1, amplitudekey[0], reinterpret_cast<const float * __restrict>(&amplitudekey[1]), time, amplitudehint);
		amplitude = xs_RoundToInt(amplitude / amplitudequant) * amplitudequant;

		// get current offset value
		float offset;
		offsetfunc(&offset, 1, offsetkey[0], reinterpret_cast<const float * __restrict>(&offsetkey[1]), time, offsethint);
		offset = xs_RoundToInt(offset / offsetquant) * offsetquant;

		// advance time
		time += steptime;
#endif

		// accumulator
		float accum = 0.0f;

		// remaining subsample ticks
		float ticks = sampleticks;

		// if a transition will happen this sample...
		while (ticks >= divider - counter)
		{
			// accumulate value over interval
			if (outputhigh)
				accum += divider - counter;

			// use up ticks
			ticks -= divider - counter;

			// reset counter
			counter = 0.0f;

			// perform one update tick
			if ((!poly1data) ||
				(poly1data[poly1index = xs_FloorToInt(poly1index + divider) % poly1size]))
			{
				if (poly2data)
					outputhigh = poly2data[poly2index = xs_FloorToInt(poly2index + divider) % poly2size];
				else
					outputhigh = !outputhigh;
			}
		}

		// accumulate value over interval
		if (outputhigh)
			accum += ticks;

		// use up remaining fraction
		counter += ticks;

		// normalize accumulator to [-1..1]
		accum *= ticksamples;
		accum += accum;
		accum -= 1.0f;

		// compute sample value
		short sample = short(Clamp(xs_RoundToInt(SHRT_MAX * (offset + amplitude * accum)), SHRT_MIN, SHRT_MAX));

		// append sample
		self.Append(sample);
	}

	return true;
}
}
soundpokeyloader;
