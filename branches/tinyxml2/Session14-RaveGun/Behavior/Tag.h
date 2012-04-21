#pragma once

union Tag
{
	float f;
	int i;
	unsigned int u;
};

namespace Database
{
	extern Typed<Typed<Tag> > tag;
}
