#include "StdAfx.h"

#include "Title.h"


// convert HSV [0..1] to RGB [0..1]
static void HSV2RGB(float h, float s, float v, float &r, float &g, float &b)
{
#if 1
	// convert hue to index and fraction
	const int bits = 20;
	int scaled = (xs_FloorToInt(h * (1 << bits)) & ((1 << bits) - 1)) * 6;
	int i = scaled >> bits;
	float f = scaled * (1.0f / (1 << bits)) - i;

	// generate components
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);

	switch (i)
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	}
#else
	// http://www.xmission.com/~trevin/atari/video_notes.html
	const float Y = 0.7f, S = 0.7f, theta = float(M_PI) - float(M_PI) * (sim_turn & 63) / 32.0f;
	float R = Clamp(Y + S * sin(theta), 0.0f, 1.0f);
	float G = Clamp(Y - (27/53) * S * sin(theta) - (10/53) * S * cos(theta), 0.0f, 1.0f);
	float B = Clamp(Y + S * cos(theta), 0.0f, 1.0f);
#endif
}

#define TITLE_NONE -1
#define TITLE_DEFAULT 0
#define TITLE_ROCKETBOMB 3
#define TITLE_ASSAULTWING 4
#define TITLE_DREADNOUGHT 5
#define TITLE_BUZZKILL 7
#define TITLE_WARP8 8
#define TITLE_NOVACORE 9
#define TITLE_TEXT TITLE_DEFAULT

#if TITLE_TEXT == TITLE_NONE

static const char titlemap[][1] = { "" };
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f };

#elif TITLE_TEXT == TITLE_ROCKETBOMB

// title text bitmap
static const char titlemap[][94+1] = 
{
//   0000000001111111111222222222233333333334444444444555555555566666666667777777777888888888899999
//   1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234
	"0000000000000    000000000000    000000000000   000000  000000  00000000000000  00000000000000",
	"00000000000000  00000000000000  00000000000000  000000  000000  00000000000000  00000000000000",
	"000000  000000  000000  000000  000000  000000  000000 000000   000000              000000    ",
	"00000000000000  000000  000000  000000          000000000000    000000000           000000    ",
	"0000000000000   000000  000000  000000          000000000000    000000000           000000    ",
	"000000  000000  000000  000000  000000  000000  000000 000000   000000              000000    ",
	"000000  000000  00000000000000  00000000000000  000000  000000  00000000000000      000000    ",
	"000000  000000   000000000000    000000000000   000000  000000  00000000000000      000000    ",
	"                                                                                              ",
	"                                                                                              ",
	"111111111111111111111    11111111111111111111   111111111    111111111  111111111111111111111 ",
	"1111111111111111111111  1111111111111111111111  1111111111  1111111111  1111111111111111111111",
	"1111111111111111111111  1111111111111111111111  1111111111111111111111  1111111111111111111111",
	"1111111111  1111111111  1111111111  1111111111  1111111111111111111111  1111111111  1111111111",
	"1111111111  1111111111  1111111111  1111111111  11111111 1111 11111111  1111111111  1111111111",
	"111111111111111111111   1111111111  1111111111  111111111 11 111111111  111111111111111111111 ",
	"111111111111111111111   1111111111  1111111111  1111111111  1111111111  111111111111111111111 ",
	"1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111",
	"1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111  1111111111",
	"1111111111111111111111  1111111111111111111111  1111111111  1111111111  1111111111111111111111",
	"1111111111111111111111  1111111111111111111111  1111111111  1111111111  1111111111111111111111",
	"111111111111111111111    11111111111111111111   1111111111  1111111111  111111111111111111111 ",
}

// title bar alphas
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f };

#elif TITLE_TEXT == TITLE_ASSAULTWING

// title text bitmap
static const char titlemap[][102+1] =
{
//   00000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000
//   12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234
	" 000000000000    0000000000000   000000000000    000000000000   000000  000000  000000  00000000000000",
	"000000  000000  000000          000000          000000  000000  000000  000000  000000          000000",
	"00000000000000   000000000000    000000000000   00000000000000  000000  000000  000000          000000",
	"000000  000000          000000          000000  000000  000000  000000  000000  000000          000000",
	"000000  000000  00000000000000  00000000000000  000000  000000  00000000000000  00000000000000  000000",
	"000000  000000  00000000000000  00000000000000  000000  000000  00000000000000  00000000000000  000000",
	"000000  000000  00000000000000  00000000000000  000000  000000  00000000000000  00000000000000  000000",
	"000000  000000  0000000000000   0000000000000   000000  000000   000000000000    0000000000000  000000",
	"                                                                                                      ",
	"                                                                                                      ",
	"  11111111            11111111  11111111   1111111111111111   11111111   11111111111111111111111111   ",
	"  11111111            11111111  11111111  111111111111111111  11111111  1111111111111111111111111111  ",
	"  11111111  11111111  11111111  11111111  11111111  11111111  11111111  11111111                      ",
	"  11111111  11111111  11111111  11111111  11111111  11111111  11111111  11111111  111111111111111111  ",
	"  11111111  11111111  11111111  11111111  11111111  11111111  11111111  11111111  111111111111111111  ",
	"  11111111  11111111  11111111  11111111  11111111  11111111  11111111  11111111            11111111  ",
	"  1111111111111111111111111111  11111111  11111111  111111111111111111  1111111111111111111111111111  ",
	"  1111111111111111111111111111  11111111  11111111  111111111111111111  1111111111111111111111111111  ",
	"  1111111111111111111111111111  11111111  11111111  111111111111111111  1111111111111111111111111111  ",
	"  1111111111111111111111111111  11111111  11111111  111111111111111111  1111111111111111111111111111  ",
	"  1111111111111111111111111111  11111111  11111111  111111111111111111  1111111111111111111111111111  ",
	"   11111111111111111111111111   11111111  11111111   1111111111111111    11111111111111111111111111   ",
};

// title bar alphas
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f };

#elif TITLE_TEXT == TITLE_DREADNOUGHT

// title text bitmap
static const char titlemap[][98+1] =
{
//   00000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000
//   12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234
	"0000000  0000000  00000000  000000  0000000  000  000  000000  0000 000  000000  0000 000 00000000",
	"0000 000 0000 000 0000     0000 000 0000 000 0000 000 0000 000 0000 000 0000     0000 000   0000  ",
	"0000 000 0000000  000000   00000000 0000 000 00000000 0000 000 0000 000 0000 000 00000000   0000  ",
	"0000 000 0000 000 0000     0000 000 0000 000 00000000 0000 000 0000 000 0000 000 0000 000   0000  ",
	"00000000 0000 000 00000000 0000 000 00000000 00000000 00000000 00000000 00000000 0000 000   0000  ",
	"00000000 0000 000 00000000 0000 000 00000000 00000000 00000000 00000000 00000000 0000 000   0000  ",
	"00000000 0000 000 00000000 0000 000 00000000 000 0000 00000000 00000000 00000000 0000 000   0000  ",
	"0000000  0000 000 00000000 0000 000 0000000  000  000  000000   000000   000000  0000 000   0000  ",
	"                                                                                                  ",
	"                                                                                                  ",
	"   111111111111111   1111111111111111   11111111   111111111111111   11111111   111111111111111   ",
	"  11111111111111111  11111111111111111  11111111  11111111111111111  11111111  11111111111111111  ",
	"  11111111 11111111  11111111 11111111  11111111  11111111 11111111  11111111  11111111 11111111  ",
	"  11111111 11111111  11111111 11111111  11111111  11111111           11111111  11111111           ",
	"  11111111           1111111111111111   11111111   111111111111111   11111111   111111111111111   ",
	"  11111111 11111111  11111111 11111111  11111111           11111111  11111111           11111111  ",
	"  11111111 11111111  11111111 11111111  11111111  11111111 11111111  11111111  11111111 11111111  ",
	"  11111111 11111111  11111111 11111111  11111111  11111111111111111  11111111  11111111111111111  ",
	"  11111111111111111  11111111 11111111  11111111  11111111111111111  11111111  11111111111111111  ",
	"  11111111111111111  11111111 11111111  11111111  11111111111111111  11111111  11111111111111111  ",
	"  11111111111111111  11111111 11111111  11111111  11111111111111111  11111111  11111111111111111  ",
	"   111111111111111   11111111 11111111  11111111   111111111111111   11111111   111111111111111   ",
};

// title bar alphas
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f };

#elif TITLE_TEXT == TITLE_BUZZKILL

// title text bitmap
static const char titlemap[][82+1] =
{
//   0000000001111111111222222222233333333334444444444555555555566666666667777777777888
//   1234567890123456789012345678901234567890123456789012345678901234567890123456789012
	"  00000000000000000   00000000  00000000  000000000000000000  000000000000000000  ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000  ",
	"  00000000  00000000  00000000  00000000           000000000           000000000  ",
	"  00000000000000000   00000000  00000000   0000000000000000    0000000000000000   ",
	"  00000000  00000000  00000000  00000000  000000000           000000000           ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000  ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000  ",
	"  00000000000000000    0000000000000000   000000000000000000  000000000000000000  ",
	"                                                                                  ",
	"                                                                                  ",
	"1111111111  1111111111  1111111111  1111111111              1111111111            ",
	"1111111111  1111111111  1111111111  1111111111              1111111111            ",
	"1111111111  1111111111  1111111111  1111111111              1111111111            ",
	"1111111111 111111111    1111111111  1111111111              1111111111            ",
	"1111111111111111111     1111111111  1111111111              1111111111            ",
	"1111111111 111111111    1111111111  1111111111              1111111111            ",
	"1111111111  1111111111  1111111111  1111111111              1111111111            ",
	"1111111111  1111111111  1111111111  1111111111111111111111  1111111111111111111111",
	"1111111111  1111111111  1111111111  1111111111111111111111  1111111111111111111111",
	"1111111111  1111111111  1111111111  1111111111111111111111  1111111111111111111111",
	"1111111111  1111111111  1111111111  1111111111111111111111  1111111111111111111111",
	"1111111111  1111111111  1111111111  1111111111111111111111  1111111111111111111111",
};

// title bar alphas
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f };

#elif TITLE_TEXT == TITLE_WARP8

// title text bitmap
static const char titlemap[][92+1] =
{
//   00000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999
//   12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
	"  00000000  00000000  00000000   0000000000000000   00000000000000000   00000000000000000   ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000  ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000  ",
	"  00000000  00000000  00000000  000000000000000000  00000000000000000   00000000000000000   ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000            ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000            ",
	"  00000000  00000000  00000000  00000000  00000000  00000000  00000000  00000000            ",
	"   00000000000000000000000000   00000000  00000000  00000000  00000000  00000000            ",
	"                                                                                            ",
	"                                                                                            ",
	"                                    11111111111111111111                                    ",
	"                                   1111111111  1111111111                                   ",
	"  0000000000000000000000000000000  1111111111  1111111111  0000000000000000000000000000000  ",
	"                                   1111111111  1111111111                                   ",
	"    00000000000000000000000000000  1111111111  1111111111  00000000000000000000000000000    ",
	"                                    11111111111111111111                                    ",
	"      000000000000000000000000000  1111111111  1111111111  000000000000000000000000000      ",
	"                                   1111111111  1111111111                                   ",
	"        0000000000000000000000000  1111111111  1111111111  0000000000000000000000000        ",
	"                                   1111111111  1111111111                                   ",
	"          00000000000000000000000  1111111111  1111111111  00000000000000000000000          ",
	"                                   1111111111  1111111111                                   ",
	"                                    11111111111111111111                                    ",
};

// title bar alphas
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f };

#elif TITLE_TEXT == TITLE_NOVACORE

// title text bitmap
static const char titlemap[][82+1] =
{
//   00000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999
//   12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678
	"                            000000                                                ",
	"                         000000000000                                             ",
	"00000000000000000       00000    00000      00000000  00000000   0000000000000000 ",
	"00000000  00000000     000          000     00000000  00000000  00000000  00000000",
	"00000000  00000000    000            000    00000000  00000000            00000000",
	"00000000  00000000   000     1111     000   00000000  00000000   00000000000000000",
	"00000000  00000000   00    11111111    00   00000000  00000000  00000000  00000000",
	"00000000  00000000   00   1111111111   00   00000000  00000000  00000000  00000000",
	"00000000  00000000  000   1111111111   000   0000000  0000000   00000000  00000000",
	"00000000  00000000  00   111111111111   00    00000000000000     00000000000000000",
	"                    00   111111111111   00                                        ",
	"                    00   111111111111   00                                        ",
	" 0000000000000000   00   111111111111   00   0000000000000000    0000000000000000 ",
	"00000000  00000000  000   1111111111   000  00000000  00000000  00000000  00000000",
	"00000000  00000000   00   1111111111   00   00000000  00000000  00000000  00000000",
	"00000000             00    11111111    00   00000000            000000000000000000",
	"00000000  00000000   000     1111     000   00000000            00000000          ",
	"00000000  00000000    000            000    00000000            00000000  00000000",
	"00000000  00000000     000          000     00000000            00000000  00000000",
	" 0000000000000000       00000    00000      00000000             0000000000000000 ",
	"                         000000000000                                             ",
	"                            000000                                                ",
};

// title bar alphas
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f, 0.0f, 0.0f };

#else

// title text bitmap
static const char titlemap[][96+1] = 
{
//   000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999
//   123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
	"    00000000  00000000  00000000  00000000000000000   000000000000000000   0000000000000000     ",
	"    00000000  00000000  00000000  000000000000000000  000000000000000000  000000000000000000    ",
	"    00000000  00000000  00000000  000000000000000000  000000000000000000  000000000000000000    ",
	"    00000000  00000000  00000000  00000000  00000000  00000000            00000000  00000000    ",
	"    00000000  00000000  00000000  00000000  00000000  00000000            00000000  00000000    ",
	"    00000000  00000000  00000000  00000000  00000000  0000000000000000    00000000  00000000    ",
	"    00000000  00000000  00000000  00000000  00000000  0000000000000000    00000000  00000000    ",
	"    00000000  00000000  00000000  00000000  00000000  00000000            00000000  00000000    ",
	"     0000000000000000   00000000  00000000  00000000  00000000            00000000  00000000    ",
	"      00000000000000    00000000  000000000000000000  000000000000000000  000000000000000000    ",
	"       000000000000     00000000  000000000000000000  000000000000000000  000000000000000000    ",
	"        0000000000      00000000  00000000000000000   000000000000000000   0000000000000000     ",
	"                                                                                                ",
	"                                                                                                ",
	"11111  11111  111111111111  11111  11111  111111111111  11111  11111  11111111111   111111111111",
	"11111  11111  111111111111  11111  11111  111111111111  11111  11111  111111111111  111111111111",
	"11111  11111  11111         111111 11111     111111     11111  11111  11111  11111  11111       ",
	"11111  11111  1111111111    111111111111     111111     11111  11111  111111111111  1111111111  ",
	"11111  11111  1111111111    111111111111     111111     11111  11111  11111111111   1111111111  ",
	" 1111111111   11111         11111 111111     111111     11111  11111  11111  11111  11111       ",
	"  11111111    111111111111  11111  11111     111111     111111111111  11111  11111  111111111111",
	"   111111     111111111111  11111  11111     111111      1111111111   11111  11111  111111111111",
};

// title bar alphas
static float baralpha[SDL_arraysize(titlemap)] = { 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f};

#endif


// border drawing properties
static const float borderw = 2;
static const float borderh = 2;

// title drawing properties
static const float titlew = 6;
static const float titleh = 6;
static const float titlex = 320 - titlew * 0.5f * (SDL_arraysize(titlemap[0]) - 1);
static const float titley = 16;
static const float titlez = 0;

// border rectangles
enum BorderCorner
{
	BORDER_UL,
	BORDER_U,
	BORDER_UR,
	BORDER_L,
	BORDER_C,
	BORDER_R,
	BORDER_BL,
	BORDER_B,
	BORDER_BR
};
static const float block[9][2][2] =
{
	{ { 0, borderw }, { 0, borderh } },
	{ { borderw, titlew - borderw }, { 0, borderh } },
	{ { titlew - borderw, titlew }, { 0, borderh } },
	{ { 0, borderw }, { borderh, titleh - borderh } },
	{ { 0, titlew }, { 0, titleh } },	// <-- filled block
	{ { titlew - borderw, titlew }, { borderh, titleh - borderh } },
	{ { 0, borderw }, { titleh - borderh, titleh } },
	{ { borderw, titlew - borderh}, { titleh - borderh, titleh } },
	{ { titlew - borderw, titlew }, { titleh - borderh, titleh } },
};
static const int mask[9] =
{
	(1<<BORDER_UL), ((1<<BORDER_UL)|(1<<BORDER_U)|(1<<BORDER_UR)), (1<<BORDER_UR),
	((1<<BORDER_UL)|(1<<BORDER_L)|(1<<BORDER_BL)), (1 << BORDER_C), ((1<<BORDER_UR)|(1<<BORDER_R)|(1<<BORDER_BR)),
	(1<<BORDER_BL), ((1<<BORDER_BL)|(1<<BORDER_B)|(1<<BORDER_BR)), (1<<BORDER_BR)
};

#define USE_TITLE_MIRROR_WATER_EFFECT
#ifdef USE_TITLE_MIRROR_WATER_EFFECT
// mirror offset
static const float mirrorscale = -0.75f;
static const float titleheight = titleh * (SDL_arraysize(titlemap) + 1);
static const float mirrortop = titley + titleheight + titleh * 2 + 4;
static const float mirrorbottom = mirrortop - mirrorscale * titleheight;
static const float mirroralphadelta = -0.375f / 32;
static const float mirroralphastart = 0.375f - mirroralphadelta * mirrortop;
#endif

ShellTitle::ShellTitle(unsigned int aId)
	: Overlay(aId)
{
	titlefill = new unsigned short[(SDL_arraysize(titlemap) + 2) * (SDL_arraysize(titlemap[0]) + 1)];

	SetAction(Action(this, &ShellTitle::Render));

	// generate fill data
	unsigned short *titlefillptr = titlefill;
	for (int row = -1; row < (int)SDL_arraysize(titlemap) + 1; ++row)
	{
		for (int col = -1; col < (int)SDL_arraysize(titlemap[0]); ++col)
		{
			int phase = 0;
			int fill = 0;

			int c0 = std::max<int>(col - 1, 0);
			int c1 = std::min<int>(col + 1, SDL_arraysize(titlemap[0]) - 2);
			int r0 = std::max<int>(row - 1, 0);
			int r1 = std::min<int>(row + 1, SDL_arraysize(titlemap) - 1);

			for (int r = r0; r <= r1; ++r)
			{
				for (int c = c0; c <= c1; ++c)
				{
					if (titlemap[r][c] >= '0')
					{
						phase = titlemap[r][c] - '0';
						fill |= mask[(r - row + 1) * 3 + (c - col + 1)];
					}
				}
			}

			if (fill & (1<<4))
				fill = (1<<4);

			*titlefillptr++ = unsigned short(fill | (phase << 9));
		}
	}
}

ShellTitle::~ShellTitle()
{
	delete[] titlefill;
}

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
// mirror y-axis wave function
static float MirrorWaveY(float y)
{
	return mirrorbottom + mirrorscale * y + 1.0f * sinf(sim_turn / 64.0f + y / 8.0f) + 3.0f * sinf(sim_turn / 128.0f + y / 32.0f);
}

// mirror x-axis wave function
static float MirrorWaveX(float y)
{
	return 1.0f * sinf(sim_turn / 32.0f + y / 4.0f);
}
#endif

// block color
static float BlockHue(int col, int row)
{
	return sim_turn / 1024.0f + row / 128.0f + 0.03125f * sinf(sim_turn / 64.0f + row / 4.0f + 4.0f * sinf(sim_turn / 64.0f + col / 8.0f + 0.5f * sinf(sim_turn / 64.0f + row / 4.0f)));
}

// draw title
void ShellTitle::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
//#define USE_TITLE_VERTEX_ARRAY
#ifdef USE_TITLE_VERTEX_ARRAY
	static Vector2 vertexarray[32768];
	static unsigned int colorarray[32768];
	Vector2 *vertexptr = vertexarray;
	unsigned int *colorptr = colorarray;
#else
	glBegin(GL_QUADS);
#endif

	// draw title bar
	for (int row = 0; row < SDL_arraysize(titlemap); ++row)
	{
		float y0 = titley + row * titleh, y1 = y0 + titleh;

#ifdef USE_TITLE_VERTEX_ARRAY
		unsigned int color = (xs_RoundToInt(255*baralpha[row]) << 24) | 0x00505050;
		*colorptr++ = color;
		*colorptr++ = color;
		*colorptr++ = color;
		*colorptr++ = color;
		*vertexptr++ = Vector2(0, y0);
		*vertexptr++ = Vector2(640, y0);
		*vertexptr++ = Vector2(640, y1);
		*vertexptr++ = Vector2(0, y1);
#else
		glColor4f(0.3f, 0.3f, 0.3f, baralpha[row]);
		glVertex2f(0, y0);
		glVertex2f(640, y0);
		glVertex2f(640, y1);
		glVertex2f(0, y1);
#endif
	}

	// draw title body
	unsigned short *titlefillptr = titlefill;

#if 1
#ifdef USE_TITLE_MIRROR_WATER_EFFECT
	// starting mirror properties
	float mirror_y0 = MirrorWaveY(titley - titleh);
	float mirror_d0 = MirrorWaveX(mirror_y0);
	float mirror_a0 = mirroralphastart + mirroralphadelta * mirror_y0;
#endif

	for (int row = -1; row < (int)SDL_arraysize(titlemap) + 1; ++row)
	{
		float y = titley + row * titleh;

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
		// row mirror properties
		float mirror_y1 = MirrorWaveY(y + titleh);
		float mirror_yd = (mirror_y1 - mirror_y0) / titleh;
		float mirror_d1 = MirrorWaveX(mirror_y1);
		float mirror_dd = (mirror_d1 - mirror_d0) / titleh;
		float mirror_a1 = mirroralphastart + mirroralphadelta * mirror_y1;
		float mirror_ad = (mirror_a1 - mirror_a0) / titleh;
#endif

		for (int col = -1; col < (int)SDL_arraysize(titlemap[0]); ++col)
		{
			float x = titlex + col * titlew;

			if (*titlefillptr != 0)
			{
				int phase = *titlefillptr >> 9;
				int fill = *titlefillptr & 0x1FF;

				// get block color
				float R, G, B;
				float h = BlockHue(col, row);
				bool border = (fill & ~(1<<4)) != 0;
				HSV2RGB(h + phase * 0.5f + border * 0.5f, 1.0f, 1.0f - 0.25f * border, R, G, B);

				// for each block...
				for (int i = 0; i < 9; ++i)
				{
					// if the block is filled
					if (fill & (1 << i))
					{
						// block borders
						float x0 = x + block[i][0][0];
						float x1 = x + block[i][0][1];
						float y0 = y + block[i][1][0];
						float y1 = y + block[i][1][1];

						// upright
#ifdef USE_TITLE_VERTEX_ARRAY
						unsigned int color = 0xFF000000 | (xs_RoundToInt(B * 255) << 16) | (xs_RoundToInt(G * 255) << 8) | (xs_RoundToInt(R * 255) );
						*colorptr++ = color;
						*colorptr++ = color;
						*colorptr++ = color;
						*colorptr++ = color;
						*vertexptr++ = Vector2(x0, y0);
						*vertexptr++ = Vector2(x1, y0);
						*vertexptr++ = Vector2(x1, y1);
						*vertexptr++ = Vector2(x0, y1);
#else
						glColor4f(R, G, B, 1.0f);
						glVertex2f(x0, y0);
						glVertex2f(x1, y0);
						glVertex2f(x1, y1);
						glVertex2f(x0, y1);
#endif

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
						if (mirror_a0 > 0.0f || mirror_a1 > 0.0f)
						{
							// mirrored
							float m0 = y0 - y;
							float m1 = y1 - y;
							float a0 = std::max(mirror_a0 + mirror_ad * m0, 0.0f);
							float a1 = std::max(mirror_a0 + mirror_ad * m1, 0.0f);
							float dx0 = mirror_d0 + mirror_dd * m0;
							float dx1 = mirror_d0 + mirror_dd * m1;
							float yy0 = mirror_y0 + mirror_yd * m0;
							float yy1 = mirror_y0 + mirror_yd * m1;
#ifdef USE_TITLE_VERTEX_ARRAY
							color &= 0x00FFFFFF;
							color |= xs_RoundToInt(a1 * a1 * 255) << 24;
							*colorptr++ = color;
							*colorptr++ = color;
							*vertexptr++ = Vector2(x0 + dx1, yy1);
							*vertexptr++ = Vector2(x1 + dx1, yy1);
							color &= 0x00FFFFFF;
							color |= xs_RoundToInt(a0 * a0 * 255) << 24;
							*colorptr++ = color;
							*colorptr++ = color;
							*vertexptr++ = Vector2(x1 + dx0, yy0);
							*vertexptr++ = Vector2(x0 + dx0, yy0);
#else
							glColor4f(R, G, B, a1 * a1);
							glVertex2f(x0 + dx1, yy1);
							glVertex2f(x1 + dx1, yy1);
							glColor4f(R, G, B, a0 * a0);
							glVertex2f(x1 + dx0, yy0);
							glVertex2f(x0 + dx0, yy0);
#endif
						}
#endif
					}
				}
			}

			++titlefillptr;
		}

#ifdef USE_TITLE_MIRROR_WATER_EFFECT
		// mirror shift row
		mirror_y0 = mirror_y1;
		mirror_d0 = mirror_d1;
		mirror_a0 = mirror_a1;
#endif
	}

#ifdef USE_TITLE_VERTEX_ARRAY
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertexarray);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, colorarray);
	glDrawArrays(GL_QUADS, 0, vertexptr - vertexarray);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
#else
	glEnd();
#endif

#else
	// texture-based variant

	glEnd();

	glEnable(GL_TEXTURE_2D);

	static const int titletexwidth = 128;
	static const int titletexheight = 64;
	static const float titleborderu = float(borderw) / float(titlew * titletexwidth);
	static const float titleborderv = float(borderh) / float(titleh * titletexheight);

	static GLuint titletexture = 0;
	if (titletexture == 0)
	{
		glGenTextures(1, &titletexture);
		{int err=glGetError();if(err)DebugPrint("glGenTextures() error: %i\n",err);}
	}

	// bind title texture
	glBindTexture(GL_TEXTURE_2D, titletexture);
	{int err=glGetError();if(err)DebugPrint("glBindTexture() error: %i\n",err);}
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// generate texture data
	unsigned char texturedata[titletexheight][titletexwidth][3];
	for (int row = -1; row < (int)SDL_arraysize(titlemap) + 1; ++row)
	{
		for (int col = -1; col < (int)SDL_arraysize(titlemap[0]); ++col)
		{
			if (*titlefillptr != 0)
			{
				int phase = *titlefillptr >> 9;
				int fill = *titlefillptr & 0x1FF;

				// get block color
				float R, G, B;
				float h = BlockHue(col, row);
				bool border = (fill & ~(1<<4)) != 0;
				HSV2RGB(h + phase * 0.5f + border * 0.5f, 1.0f, 1.0f - 0.25f * border, R, G, B);

				texturedata[row+1][col+1][0] = (unsigned char)(int)(R * 255);
				texturedata[row+1][col+1][1] = (unsigned char)(int)(G * 255);
				texturedata[row+1][col+1][2] = (unsigned char)(int)(B * 255);
			}
			else
			{
				texturedata[row+1][col+1][0] = 0;
				texturedata[row+1][col+1][1] = 0;
				texturedata[row+1][col+1][2] = 0;
			}

			++titlefillptr;
		}
	}

	// upload texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, titletexwidth, titletexheight, 0, GL_RGB, GL_UNSIGNED_BYTE, texturedata);
	{int err=glGetError();if(err)DebugPrint("glTexImage2D() error: %i\n",err);}

	// if no title slabs generated...
	static unsigned char titleslabmap[SDL_arraysize(titlemap)][SDL_arraysize(titlemap[0])-1];
	static int titleslab[255][4];
	static int titleslabcount = 0;
	if (titleslabcount == 0)
	{
		// initialize slab map
		memset(&titleslabmap[0][0], 0xFF, sizeof(titleslabmap));

		// generate title slabs
		for (int row = 0; row < SDL_arraysize(titlemap); ++row)
		{
			for (int col = 0; col < SDL_arraysize(titlemap[row])-1; ++col)
			{
				// skip empty spaces
				if (titlemap[row][col] == ' ')
					continue;

				// skip assigned spaces
				if (titleslabmap[row][col] != 0xFF)
					continue;

				// allocate a new index
				int index = titleslabcount++;

				// find horizontal extent
				int c0 = col;
				int c1 = SDL_arraysize(titlemap[row]) - 1;
				for (int c = c0; c < c1; ++c)
				{
					if ((titlemap[row][c] == ' ') || ((titleslabmap[row][c] != 0xFF) && (titleslabmap[row][c] != index)))
					{
						c1 = c;
						break;
					}
				}

				// find vertical extent
				int r0 = row;
				int r1 = SDL_arraysize(titlemap);
				for (int r = r0; r < r1; ++r)
				{
					for (int c = c0; c < c1; ++c)
					{
						if ((titlemap[r][c] == ' ') || ((titleslabmap[r][c] != 0xFF) && (titleslabmap[r][c] != index)))
						{
							r1 = r;
							break;
						}
					}
				}

				// fill slab
				for (int r = r0; r < r1; ++r)
				{
					for (int c = c0; c < c1; ++c)
					{
						titleslabmap[r][c] = (unsigned char)index;
					}
				}

				assert(c0 < c1 && r0 < r1);

				// set slab extents
				titleslab[index][0] = c0;
				titleslab[index][1] = c1;
				titleslab[index][2] = r0;
				titleslab[index][3] = r1;

				// skip visited columns
				col = c1;
			}
		}
	}

	// draw title body
	glBegin(GL_QUADS);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// for each title slab...
	for (int i = 0; i < titleslabcount; ++i)
	{
		// get slab extents
		int c0 = titleslab[i][0];
		int c1 = titleslab[i][1];
		int r0 = titleslab[i][2];
		int r1 = titleslab[i][3];

		// generate texture extents
		float u0 = float(c0+1) / titletexwidth - titleborderu, u1 = float(c1+1) / titletexwidth + titleborderu;
		float v0 = float(r0+1) / titletexheight - titleborderv, v1 = float(r1+1) / titletexheight + titleborderv;

		// generate position extents
		float x0 = titlex + c0 * titlew - borderw, x1 = titlex + c1 * titlew + borderw;
		float y0 = titley + r0 * titleh - borderh, y1 = titley + r1 * titleh + borderh;

		// submit vertices
		glTexCoord2f(u0, v0);	glVertex2f(x0, y0);
		glTexCoord2f(u1, v0);	glVertex2f(x1, y0);
		glTexCoord2f(u1, v1);	glVertex2f(x1, y1);
		glTexCoord2f(u0, v1);	glVertex2f(x0, y1);
	}

	glEnd();

	glDisable(GL_TEXTURE_2D);

#endif
}
