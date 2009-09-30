#include "StdAfx.h"
#include "Noise.h"

// Implementation based on Ken Perlin's Java implementation and Stefan Gustavson C++ "Noise1234" implementation

// Ken Perlin's improved interpolant
static inline float Fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

// Ken Perlin's permutation table
static unsigned char p[256] =
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
static inline unsigned char Permute(register unsigned char i)
{
	return p[i];
}

// 1D gradient
// (16 slopes between -1.5 and +1.5)
static float Gradient(int hash, float x)
{
	static const float g[16] =
	{
		  1.f*0.1875f,  2.f*0.1875f,  3.f*0.1875f,  4.f*0.1875f,  5.f*0.1875f,  6.f*0.1875f,  7.f*0.1875f,  8.f*0.1875f,
		 -1.f*0.1875f, -2.f*0.1875f, -3.f*0.1875f, -4.f*0.1875f, -5.f*0.1875f, -6.f*0.1875f, -7.f*0.1875f, -8.f*0.1875f,
	};
	return g[hash & 15] * x;
}

// 2D gradient
// (8 directions similar to moves of the knight chess piece)
static float Gradient(int hash, float x, float y)
{
	float u = (hash & 4) ? y : x;
	float v = (hash & 4) ? x : y;
	return ((hash & 1)? -u : u) + ((hash & 2)? -v-v : v+v);
}

// 3D gradient
// (12 vectors to the edges of a cube and 4 repeats)
static float Gradient(int hash, float x, float y , float z)
{
	int h = hash & 15;
	float u = h < 8 ? x : y;
	float v = h < 4 ? y : (h==12 || h==14 ? x : z);
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

// 4D gradient
// (32 vectors to the edges of a 4D hypercube)
static float Gradient(int hash, float x, float y, float z, float t)
{
	int h = hash & 31;
	float u = h < 24 ? x : y;
	float v = h < 16 ? y : z;
	float w = h < 8 ? z : t;
	return ((h & 1)? -u : u) + ((h & 2)? -v : v) + ((h & 4)? -w : w);
}


// 1D Perlin Noise
float Noise(float x)
{
	// split x value into integer, fraction, and interpolator parts
	int ix = xs_FloorToInt(x);
	float fx0 = x - ix;
	float fx1 = fx0 - 1.0f;
	unsigned char ix0 = unsigned char(ix);
	unsigned char ix1 = unsigned char(ix + 1);
	float tx = Fade(fx0);

	// interpolate along x axis
	float nx0 = Gradient(Permute(ix0), fx0);
	float nx1 = Gradient(Permute(ix1), fx1);
	return Lerp(nx0, nx1, tx);
}


// 2D Perlin Noise
float Noise(float x, float y)
{
	// split x value into integer, fraction, and interpolator parts
	int ix = xs_FloorToInt(x);
	float fx0 = x - ix;
	float fx1 = fx0 - 1.0f;
	unsigned char ix0 = unsigned char(ix);
	unsigned char ix1 = unsigned char(ix + 1);
	float tx = Fade(fx0);

	// split y value into integer, fraction, and interpolator parts
	int iy = xs_FloorToInt(y);
	float fy0 = y - iy;
	float fy1 = fy0 - 1.0f;
	unsigned char iy0 = unsigned char(iy);
	unsigned char iy1 = unsigned char(iy + 1);
	float ty = Fade(fy0);

	// interpolate along y axis at fx=0
	float nx0y0 = Gradient(Permute(ix0 + Permute(iy0)), fx0, fy0);
	float nx0y1 = Gradient(Permute(ix0 + Permute(iy1)), fx0, fy1);
	float nx0 = Lerp(nx0y0, nx0y1, ty);

	// interpolate along y axis at fx=1
	float nx1y0 = Gradient(Permute(ix1 + Permute(iy0)), fx1, fy0);
	float nx1y1 = Gradient(Permute(ix1 + Permute(iy1)), fx1, fy1);
	float nx1 = Lerp(nx1y0, nx1y1, ty);

	// interpolate along x axis
	return 0.5f * Lerp(nx0, nx1, tx);
}


// 3D Perlin Noise
 float Noise(float x, float y, float z)
{
	// split x value into integer, fraction, and interpolator parts
	int ix = xs_FloorToInt(x);
	float fx0 = x - ix;
	float fx1 = fx0 - 1.0f;
	unsigned char ix0 = unsigned char(ix);
	unsigned char ix1 = unsigned char(ix + 1);
	float tx = Fade(fx0);

	// split y value into integer, fraction, and interpolator parts
	int iy = xs_FloorToInt(y);
	float fy0 = y - iy;
	float fy1 = fy0 - 1.0f;
	unsigned char iy0 = unsigned char(iy);
	unsigned char iy1 = unsigned char(iy + 1);
	float ty = Fade(fy0);

	// split z value into integer, fraction, and interpolator parts
	int iz = xs_FloorToInt(z);
	float fz0 = z - iz;
	float fz1 = fz0 - 1.0f;
	unsigned char iz0 = unsigned char(iz);
	unsigned char iz1 = unsigned char(iz + 1);
	float tz = Fade(fz0);

	// interpolate along z axis at fx=0 fy=0
	float nx0y0z0 = Gradient(Permute(ix0 + Permute(iy0 + Permute(iz0))), fx0, fy0, fz0);
	float nx0y0z1 = Gradient(Permute(ix0 + Permute(iy0 + Permute(iz1))), fx0, fy0, fz1);
	float nx0y0 = Lerp(nx0y0z0, nx0y0z1, tz);

	// interpolate along z axis at fx=0 fy=1
	float nx0y1z0 = Gradient(Permute(ix0 + Permute(iy1 + Permute(iz0))), fx0, fy1, fz0);
	float nx0y1z1 = Gradient(Permute(ix0 + Permute(iy1 + Permute(iz1))), fx0, fy1, fz1);
	float nx0y1 = Lerp(nx0y1z0, nx0y1z1, tz);

	// interpolate along y axis at fx=0
	float nx0 = Lerp(nx0y0, nx0y1, ty);

	// interpolate along z axis at fx=1 fy=0
	float nx1y0z0 = Gradient(Permute(ix1 + Permute(iy0 + Permute(iz0))), fx1, fy0, fz0);
	float nx1y0z1 = Gradient(Permute(ix1 + Permute(iy0 + Permute(iz1))), fx1, fy0, fz1);
	float nx1y0 = Lerp(nx1y0z0, nx1y0z1, tz);

	// interpolate along z axis at fx=1 fy=1
	float nx1y1z0 = Gradient(Permute(ix1 + Permute(iy1 + Permute(iz0))), fx1, fy1, fz0);
	float nx1y1z1 = Gradient(Permute(ix1 + Permute(iy1 + Permute(iz1))), fx1, fy1, fz1);
	float nx1y1 = Lerp(nx1y1z0, nx1y1z1, tz);

	// interpolate along y axis at fx=1
	float nx1 = Lerp(nx1y0, nx1y1, ty);
	
	// interpolate along x axis
	return 0.9375f * Lerp(nx0, nx1, tx);
}


// 4D Perlin Noise
float Noise(float x, float y, float z, float w)
{
	// split x value into integer, fraction, and interpolator parts
	int ix = xs_FloorToInt(x);
	float fx0 = x - ix;
	float fx1 = fx0 - 1.0f;
	unsigned char ix0 = unsigned char(ix);
	unsigned char ix1 = unsigned char(ix + 1);
	float tx = Fade(fx0);

	// split y value into integer, fraction, and interpolator parts
	int iy = xs_FloorToInt(y);
	float fy0 = y - iy;
	float fy1 = fy0 - 1.0f;
	unsigned char iy0 = unsigned char(iy);
	unsigned char iy1 = unsigned char(iy + 1);
	float ty = Fade(fy0);

	// split z value into integer, fraction, and interpolator parts
	int iz = xs_FloorToInt(z);
	float fz0 = z - iz;
	float fz1 = fz0 - 1.0f;
	unsigned char iz0 = unsigned char(iz);
	unsigned char iz1 = unsigned char(iz + 1);
	float tz = Fade(fz0);

	// split w value into integer, fraction, and interpolator parts
	int iw = xs_FloorToInt(w);
	float fw0 = w - iw;
	float fw1 = fw0 - 1.0f;
	unsigned char iw0 = unsigned char(iw);
	unsigned char iw1 = unsigned char(iw + 1);
	float tw = Fade(fw0);

	// interpolate along w axis at fx=0 fy=0 fz=0
	float nx0y0z0w0 = Gradient(Permute(ix0 + Permute(iy0 + Permute(iz0 + Permute(iw0)))), fx0, fy0, fz0, fw0);
	float nx0y0z0w1 = Gradient(Permute(ix0 + Permute(iy0 + Permute(iz0 + Permute(iw1)))), fx0, fy0, fz0, fw1);
	float nx0y0z0 = Lerp(nx0y0z0w0, nx0y0z0w1, tw);
	
	// interpolate along w axis at fx=0 fy=0 fz=1
	float nx0y0z1w0 = Gradient(Permute(ix0 + Permute(iy0 + Permute(iz1 + Permute(iw0)))), fx0, fy0, fz1, fw0);
	float nx0y0z1w1 = Gradient(Permute(ix0 + Permute(iy0 + Permute(iz1 + Permute(iw1)))), fx0, fy0, fz1, fw1);
	float nx0y0z1 = Lerp(nx0y0z1w0, nx0y0z1w1, tw);
	
	// interpolate along z axis at fx=0 fy=0
	float nx0y0 = Lerp(nx0y0z0, nx0y0z1, tz);

	// interpolate along w axis at fx=0 fy=1 fz=0
	float nx0y1z0w0 = Gradient(Permute(ix0 + Permute(iy1 + Permute(iz0 + Permute(iw0)))), fx0, fy1, fz0, fw0);
	float nx0y1z0w1 = Gradient(Permute(ix0 + Permute(iy1 + Permute(iz0 + Permute(iw1)))), fx0, fy1, fz0, fw1);
	float nx0y1z0 = Lerp(nx0y1z0w0, nx0y1z0w1, tw);
	
	// interpolate along w axis at fx=0 fy=1 fz=1
	float nx0y1z1w0 = Gradient(Permute(ix0 + Permute(iy1 + Permute(iz1 + Permute(iw0)))), fx0, fy1, fz1, fw0);
	float nx0y1z1w1 = Gradient(Permute(ix0 + Permute(iy1 + Permute(iz1 + Permute(iw1)))), fx0, fy1, fz1, fw1);
	float nx0y1z1 = Lerp(nx0y1z1w0, nx0y1z1w1, tw);

	// interpolate along z axis at fx=0 fy=1
	float nx0y1 = Lerp(nx0y1z0, nx0y1z1, tz);

	// interpolate along y axis at fx=0
	float nx0 = Lerp(nx0y0, nx0y1, ty);

	// interpolate along w axis at fx=1 fy=0 fz=0
	float nx1y0z0w0 = Gradient(Permute(ix1 + Permute(iy0 + Permute(iz0 + Permute(iw0)))), fx1, fy0, fz0, fw0);
	float nx1y0z0w1 = Gradient(Permute(ix1 + Permute(iy0 + Permute(iz0 + Permute(iw1)))), fx1, fy0, fz0, fw1);
	float nx1y0z0 = Lerp(nx1y0z0w0, nx1y0z0w1, tw);
		
	// interpolate along w axis at fx=1 fy=0 fz=1
	float nx1y0z1w0 = Gradient(Permute(ix1 + Permute(iy0 + Permute(iz1 + Permute(iw0)))), fx1, fy0, fz1, fw0);
	float nx1y0z1w1 = Gradient(Permute(ix1 + Permute(iy0 + Permute(iz1 + Permute(iw1)))), fx1, fy0, fz1, fw1);
	float nx1y0z1 = Lerp(nx1y0z1w0, nx1y0z1w1, tw);

	// interpolate along z axis at fx=1 fy=0
	float nx1y0 = Lerp(nx1y0z0, nx1y0z1, tz);

	// interpolate along w axis at fx=1 fy=1 fz=0
	float nx1y1z0w0 = Gradient(Permute(ix1 + Permute(iy1 + Permute(iz0 + Permute(iw0)))), fx1, fy1, fz0, fw0);
	float nx1y1z0w1 = Gradient(Permute(ix1 + Permute(iy1 + Permute(iz0 + Permute(iw1)))), fx1, fy1, fz0, fw1);
	float nx1y1z0 = Lerp(nx1y1z0w0, nx1y1z0w1, tw);
		
	// interpolate along w axis at fx=1 fy=1 fz=1
	float nx1y1z1w0 = Gradient(Permute(ix1 + Permute(iy1 + Permute(iz1 + Permute(iw0)))), fx1, fy1, fz1, fw0);
	float nx1y1z1w1 = Gradient(Permute(ix1 + Permute(iy1 + Permute(iz1 + Permute(iw1)))), fx1, fy1, fz1, fw1);
	float nx1y1z1 = Lerp(nx1y1z1w0, nx1y1z1w1, tw);

	// interpolate along z axis at fx=1 fy=1
	float nx1y1 = Lerp(nx1y1z0, nx1y1z1, tz);

	// interpolate along y axis at fx=1
	float nx1 = Lerp(nx1y0, nx1y1, ty);

	// interpolate along x axis
	return 0.875f * Lerp(nx0, nx1, tx);
}


/*
	// Ken Perlin's 3D Simplex Noise
	int i,j,k, A[3];
	float u,v,w;
	float noise(float x, float y, float z)
	{
		float s = (x+y+z)/3;
		i=xs_FloorToInt(x+s); j=xs_FloorToInt(y+s); k=xs_FloorToInt(z+s);
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
