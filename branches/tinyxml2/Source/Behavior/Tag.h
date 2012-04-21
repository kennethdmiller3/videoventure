#pragma once

union Tag
{
	float f;
	int i;
	unsigned int u;
};

namespace Database
{
	extern GAME_API Typed<Typed<Tag> > tag;
}
