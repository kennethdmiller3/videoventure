// http://aggregate.org/MAGIC/

inline float fast_fabs(register float x)
{
	return *(float *)(*(((int *)&x)+1)&0x7FFFFFFF);
}

inline unsigned int align_down(register unsigned int a, register unsigned int b)
{
	return a & ~(b - 1);
}

inline unsigned int align_up(register unsigned int a, register unsigned int b)
{
	return (a + b - 1) & -b;
}

inline unsigned int average(register unsigned int x, register unsigned int y)
{
	return (x & y) + ((x ^ y) >> 1);
}

inline unsigned int bit_reverse(register unsigned int x)
{
	register unsigned int y = 0x55555555;
	x = (((x >> 1) & y) | ((x & y) << 1));
	y = 0x33333333;
	x = (((x >> 2) & y) | ((x & y) << 2));
	y = 0x0f0f0f0f;
	x = (((x >> 4) & y) | ((x & y) << 4));
	y = 0x00ff00ff;
	x = (((x >> 8) & y) | ((x & y) << 8));
	return((x >> 16) | (x << 16));
}

inline int int_min(register int x, register int y)
{
	return x + (((y - x) >> (sizeof(int)*8-1)) & (y - x));
}

inline int int_max(register int x, register int y)
{
	return x - (((x - y) >> (sizeof(int)*8-1)) & (x - y));
}

inline bool is_pow2(register unsigned int x)
{
	return (x & (x - 1)) == 0;
}

inline unsigned int count_ones(register unsigned int x)
{
	/* 32-bit recursive reduction using SWAR...
	but first step is mapping 2-bit values
	into sum of 2 1-bit values in sneaky way
	*/
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	return(x & 0x0000003f);
}

inline unsigned int leading_zero_count(register unsigned int x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return(sizeof(int)*8 - count_ones(x));
}

inline unsigned int trailing_zero_count(register unsigned int x)
{
	return count_ones((x & -x) - 1);
}

inline unsigned int least_significant_one(register unsigned int x)
{
	return x & -x;
}

inline unsigned int most_significant_one(register unsigned int x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return(x & ~(x >> 1));
}

inline unsigned int next_pow2(register unsigned int x)
{
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return(x+1);
}

inline unsigned int floor_log2(register unsigned int x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return(count_ones(x) - 1);
}

inline unsigned int log2(register unsigned int x)
{
	register int y = (x & (x - 1));
	y |= -y;
	y >>= (sizeof(unsigned int)*8 - 1);
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return(count_ones(x) - 1 - y);
}
