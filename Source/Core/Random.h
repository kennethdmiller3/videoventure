#pragma once 

#pragma optimize("t", on)

#define RANDOM_XORSHIFT_32 0
#define RANDOM_XORSHIFTSTAR_64_32 1
#define RANDOM_PCG_RXS_M_XS_32_32 2
#define RANDOM_PCG_XSH_RR_64_32 3

#define RANDOM_TYPE RANDOM_XORSHIFT_32

namespace Random
{
	// TO DO: allow multiple random number generators

	// Xorshift-32 Random Number Generation by George Marsaglia
	// https://en.wikipedia.org/wiki/Xorshift

	// PCG Random Number Generation by Melissa O'Neill
	// https://en.wikipedia.org/wiki/Permuted_congruential_generator

#if RANDOM_TYPE == RANDOM_PCG_XSL_RR_64_32 || RANDOM_TYPE == RANDOM_XORSHIFTSTAR_64_32
	using StateType = uint64_t;
#else
	using StateType = uint32_t;
#endif
	using OutputType = uint32_t;

	// a default initial state seed
#if RANDOM_TYPE == RANDOM_XORSHIFT_32
	constexpr StateType kInitialState = 0x92D68CA2u;
#elif RANDOM_TYPE == RANDOM_XORSHIFTSTAR_64_32;
	constexpr StateType kInitialState = 0x139408DCBBF7A44ull;
#elif RANDOM_TYPE == RANDOM_PCG_RXS_M_XS_32_32
	constexpr StateType kInitialState = 0x46B56677u;
#elif RANDOM_TYPE == RANDOM_PCG_XSH_RR_64_32
	constexpr StateType kInitialState = 0x4D595DF4D0F33173ull;
#else
#error "Unknown random generator type"
#endif

	// random seed
	extern GAME_API StateType gState;

	inline OutputType Int();

	// seed the generator
	inline void Seed(StateType aState)
	{
		gState = aState;
		(void)Int();
	}

	// random unsigned long
	inline OutputType Int()
	{
		// generate output from previous state so update and output can run in parallel
		const StateType state = gState;
#if RANDOM_TYPE == RANDOM_XORSHIFT_32
		// XorShift 32
		gState ^= (gState << 13u);
		gState ^= (gState >> 17u);
		gState ^= (gState << 5u);
		return uint32_t(state);
#elif RANDOM_TYPE == RANDOM_XORSHIFTSTAR_64_32
		// XorShift* 64/32
		gState ^= (gState >> 12u);
		gState ^= (gState << 25u);
		gState ^= (gState >> 27u);
		return uint32_t(state * 0x2545F4914F6CDD1Dull >> 32u);
#elif RANDOM_TYPE == RANDOM_PCG_RXS_M_XS_32_32
		// PCG 32/32
		gState = gState * 747796405U + 47989u;
		const uint32_t value = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (value >> 22u) ^ value;
#elif RANDOM_TYPE == RANDOM_PCG_XSH_RR_64_32
		// PCG 64/32
		gState = gState * 6364136223846793005ull + 1442695040888963407ull;
		return _rotr(uint32_t(((state >> 18u) ^ state) >> 27u), uint32_t(state >> 59u));
#else
#error "Unknown random generator type"
#endif
	}

	// random uniform float
	inline float Float()
	{
#if 0
		// multiplicative conversion
		constexpr float scale = 1.f / 4294967296.f;
		return Int() * scale;
#else
		// fast conversion
		// (this version loses the last bit of the mantissa)
		union { float f; unsigned u; } floatint;
		floatint.u = 0x3f800000 | (Int() >> 9);
		return floatint.f - 1.0f;
#endif
	}

	// random range value
	inline float Value(float aAverage, float aVariance)
	{
		return (2.0f * Float() - 1.0f) * aVariance + aAverage;
	}
}

#pragma optimize("", on)
