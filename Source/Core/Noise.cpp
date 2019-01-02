#include "StdAfx.h"
#include "Noise.h"

// Implementation based on Ken Perlin's Java implementation
// and Stefan Gustavson C++ "Noise1234" implementation

// Computing vertex gradients in a loop produces much smaller code
// compared to the original implementation though it may be slightly
// slower.  It adds loop overhead but lets the compiler fully inline
// the Gradient function, eliminating function call overhead.

// implement 4D noise?
// (nothing uses it yet so disable it to save ~600 bytes)
//#define NOISE_4D

// Ken Perlin's improved C(2) continuous interpolant
static inline float Fade(float t)
{
	return ((6 * t - 15) * t + 10) * t * t * t;
}

// Ken Perlin's permutation table
static const unsigned char p[256] =
{
	151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
	140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
	247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
	 57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
	 74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
	 60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
	 65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
	200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
	 52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
	207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
	119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
	129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
	218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
	 81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
	184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
	222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180
};
static inline unsigned char Permute(unsigned char i)
{
	return p[i];
}

// 1D gradient
// (16 slopes, -8..-1, 1..8)
static inline float Gradient(unsigned char hash, float x)
{
	// convert hash to a signed 4-bit value (-8..7)
	// this is equivalent to assigning hash to a signed bitfield
	// though it assumes the >> 4 produces an arithmetic shift right
	register signed char g = signed char(signed char(hash) << 4) >> 4;

	// add one if it's 0 or more
	g += (g >= 0);

	// apply the gradient
	return g * x;
}

// 2D gradient
// (8 directions similar to moves of the knight chess piece)
static float Gradient(unsigned char hash, float x, float y)
{
	register float u, v;
	if (hash & 4)
		u = y, v = x;
	else
		u = x, v = y;
	if (hash & 2)
		v = -v;
	if (hash & 1)
		u = -u;
	return u + v + v;
}

// 3D gradient
// (12 vectors to the edges of a cube and 4 repeats)
static float Gradient(unsigned char hash, float x, float y , float z)
{
	const unsigned char h = hash & 15;
	register float u = h < 8 ? x : y;
	register float v = h < 4 ? y : (h==12 || h==14 ? x : z);
	if (h & 2)
		v = -v;
	if (h & 1)
		u = -u;
	return u + v;
}

#ifdef NOISE_4D
// 4D gradient
// (32 vectors to the edges of a 4D hypercube)
static float Gradient(unsigned char hash, float x, float y, float z, float t)
{
	const unsigned char h = hash & 31;
	register float u = h < 24 ? x : y;
	register float v = h < 16 ? y : z;
	register float w = h < 8 ? z : t;
	if (h & 4)
		w = -w;
	if (h & 2)
		v = -v;
	if (h & 1)
		u = -u;
	return u + v + w;
}
#endif

// 1D Perlin Noise
float Noise1D(float x)
{
	// split value into integer, fraction, and interpolator parts
	const int ix = int(floorf(x));
	const float fx = x - ix;
	const float tx = Fade(fx);

	// compute gradient at each vertex and interpolate
	return 0.1875f * Lerp(
		Gradient(Permute(unsigned char(ix)), fx),
		Gradient(Permute(unsigned char(ix + 1)), fx - 1.0f),
		tx);
}


// 2D Perlin Noise
float Noise2D(float x, float y)
{
	// split values into integer, fraction, and interpolator parts
	const int ix = int(floorf(x)), iy = int(floorf(y));
	const float fx = x - ix, fy = y - iy;
	const float tx = Fade(fx), ty = Fade(fy);

#if 1
	// this variant has fewer branches but the unrolled interpolate code is slightly larger
	float n[4];
	for (register int i = 0; i < 4; ++i)
	{
		// compute vertex offset from the vertex index
		register const int dx = (i) & 1, dy = (i >> 1) & 1;

		// compute gradient at the vertex
		n[i] = Gradient(Permute(unsigned char(ix + dx) + Permute(unsigned char(iy + dy))), fx - dx, fy - dy);
	}

	// interpolate
	return 0.507f * 
		Lerp(
		Lerp(n[0], n[1], tx),
		Lerp(n[2], n[3], tx),
		ty
		);
#else
	float nx, ny;
	for (register int i = 0; i < 4; ++i)
	{
		// compute vertex offset from the vertex index
		register const int dx = (i) & 1, dy = (i >> 1) & 1;

		// compute gradient for the vertex
		register const float n = Gradient(Permute(unsigned char(ix + dx) + Permute(unsigned char(iy + dy))), fx - dx, fy - dy);

		// incremental interpolation
		if (!dx)
		{
			nx = n * (1 - tx);
			continue;
		}
		nx += n * tx;
		if (!dy)
		{
			ny = nx * (1 - ty);
			continue;
		}
		ny += nx * ty;
	}
	return 0.507f * ny;
#endif
}


// 3D Perlin Noise
float Noise3D(float x, float y, float z)
{
	// split values into integer, fraction, and interpolator parts
	const int ix = int(floorf(x)), iy = int(floorf(y)), iz = int(floorf(z)); 
	const float fx = x - ix, fy = y - iy, fz = z - iz;
	const float tx = Fade(fx), ty = Fade(fy), tz = Fade(fz);

#if 1
	// this variant has fewer branches but the unrolled interpolate code is slightly larger
	float n[8];
	for (register int i = 0; i < 8; ++i)
	{
		// compute vertex offset from the vertex index
		register const int dx = (i) & 1, dy = (i >> 1) & 1, dz = (i >> 2) & 1;

		// compute gradient for the vertex
		n[i] = Gradient(Permute(unsigned char(ix + dx) + Permute(unsigned char(iy + dy) + Permute(unsigned char(iz + dz)))), fx - dx, fy - dy, fz - dz);
	}

	// interpolate
	return 0.9375f * 
		Lerp(
		Lerp(
		Lerp(n[0], n[1], tx),
		Lerp(n[2], n[3], tx),
		ty
		),
		Lerp(
		Lerp(n[4], n[5], tx),
		Lerp(n[6], n[7], tx),
		ty
		),
		tz
		);
#else
	// this variant produces slightly smaller code but has a lot of branches
	float nx, ny, nz;
	for (register int i = 0; i < 8; ++i)
	{
		// compute vertex offset from the vertex index
		register const int dx = (i) & 1, dy = (i >> 1) & 1, dz = (i >> 2) & 1;

		// compute gradient for the vertex
		register const float n = Gradient(Permute(unsigned char(ix + dx) + Permute(unsigned char(iy + dy) + Permute(unsigned char(iz + dz)))), fx - dx, fy - dy, fz - dz);

		// incremental interpolation
		if (!dx)
		{
			nx = n * (1 - tx);
			continue;
		}
		nx += n * tx;
		if (!dy)
		{
			ny = nx * (1 - ty);
			continue;
		}
		ny += nx * ty;
		if (!dz)
		{
			nz = ny * (1 - tz);
			continue;
		}
		nz += ny * tz;
	}
	return 0.9375f * nz;
#endif
}

#ifdef NOISE_4D

// 4D Perlin Noise
float Noise4D(float x, float y, float z, float w)

{
	// split values into integer, fraction, and interpolator parts
	const int ix = int(floorf(x)), iy = int(floorf(y)), iz = int(floorf(z)), iw = int(floorf(w));
	const float fx = x - ix, fy = y - iy, fz = z - iz, fw = w - iw;
	const float tx = Fade(fx), ty = Fade(fy), tz = Fade(fz), tw = Fade(fw);

	// this variant has fewer branches but the unrolled interpolate code is slightly larger
	float n[16];
	for (register int i = 0; i < 16; ++i)
	{
		// compute vertex offset from the vertex index
		register const int dx = (i) & 1, dy = (i >> 1) & 1, dz = (i >> 2) & 1, dw = (i >> 3) & 1;

		// compute gradient for the vertex
		n[i] = Gradient(Permute(unsigned char(ix + dx) + Permute(unsigned char(iy + dy) + Permute(unsigned char(iz + dz) + Permute(unsigned char(iw + dw))))), fx - dx, fy - dy, fz - dz, fw - dw);
	}

	// interpolate
	return 0.875f * 
		Lerp(
		Lerp(
		Lerp(
		Lerp(n[0], n[1], tx),
		Lerp(n[2], n[3], tx),
		ty
		),
		Lerp(
		Lerp(n[4], n[5], tx),
		Lerp(n[6], n[7], tx),
		ty
		),
		tz
		),
		Lerp(
		Lerp(
		Lerp(n[8], n[9], tx),
		Lerp(n[10], n[11], tx),
		ty
		),
		Lerp(
		Lerp(n[12], n[13], tx),
		Lerp(n[14], n[15], tx),
		ty
		),
		tz
		),
		tw
		);
}

#else

// 4D Perlin Noise
float Noise4D(float x, float y, float z, float w)
{
	assert(false);
	return 0.0f;
}
#endif

/*
	// Ken Perlin's 3D Simplex Noise
	int i,j,k, A[3];
	float u,v,w;
	float noise(float x, float y, float z)
	{
		float s = (x+y+z)/3;
		i=int(floorf(x+s)); j=int(floorf(y+s)); k=int(floorf(z+s));
		s = (i+j+k)/6.0f; u = x-i+s; v = y-j+s; w = z-k+s;
		A[0]=A[1]=A[2]=0;
		int hi = u>=w ? u>=v ? 0 : 1 : v>=w ? 1 : 2;
		int lo = u< w ? u< v ? 0 : 1 : v< w ? 1 : 2;
		return K(hi) + K(3-hi-lo) + K(lo) + K(0);
	}
	float K(int a)
	{
		float s = (A[0]+A[1]+A[2])/6.0f;
		float x = u-A[0]+s, y = v-A[1]+s, z = w-A[2]+s, t = 0.6f-x*x-y*y-z*z;
		int h = shuffle(i+A[0],j+A[1],k+A[2]);
		A[a]++;
		if (t < 0)
			return 0;
		int b5 = h>>5 & 1, b4 = h>>4 & 1, b3 = h>>3 & 1, b2= h>>2 & 1, b = h & 3;
		float p = b==1?x:b==2?y:z, q = b==1?y:b==2?z:x, r = b==1?z:b==2?x:y;
		p = (b5==b3 ? -p : p); q = (b5==b4 ? -q : q); r = (b5!=(b4^b3) ? -r : r);
		t *= t;
		return 8 * t * t * (p + (b==0 ? q+r : b2==0 ? q : r));
	}
	int shuffle(int i, int j, int k)
	{
		return b(i,j,k,0) + b(j,k,i,1) + b(k,i,j,2) + b(i,j,k,3) +
			b(j,k,i,4) + b(k,i,j,5) + b(i,j,k,6) + b(j,k,i,7) ;
	}
	int b(int i, int j, int k, int B) { return T[b(i,B)<<2 | b(j,B)<<1 | b(k,B)]; }
	int b(int N, int B) { return N>>B & 1; }
	static const int T[] = {0x15,0x38,0x32,0x2c,0x0d,0x13,0x07,0x2a};
*/
