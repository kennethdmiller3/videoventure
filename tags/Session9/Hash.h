#pragma once

#include <ctype.h>

// fnv-1a hash
inline unsigned int Hash(const void *data, size_t len, unsigned int hash = 2166136261u)
{
	const unsigned char *d = static_cast<const unsigned char *>(data);
	const unsigned char *e = d + len;
	while (d < e)
	{
		hash ^= *d++;
		hash *= 16777619u;
	}
	return hash;
}
inline unsigned int Hash(const char *string, unsigned int hash = 2166136261u)
{
	if (string == 0)
		return 0;
	for (const char *s = string; *s != 0; ++s)
	{
		hash ^= static_cast<unsigned char>(tolower(*s));
		hash *= 16777619u;
	}
	return hash;
}

