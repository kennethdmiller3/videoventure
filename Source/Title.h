#pragma once

#include "Overlay.h"

class ShellTitle : public Overlay
{
	unsigned short *titlefill;

public:
	// constructor
	ShellTitle(unsigned int aId);

	// destructor
	~ShellTitle();

	// draw title
	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
