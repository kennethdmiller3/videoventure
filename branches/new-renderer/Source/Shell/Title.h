#pragma once

#include "Overlay.h"

class ShellTitleTemplate : public OverlayTemplate
{
	friend class ShellTitle;
	int cols;
	int rows;
	unsigned short *titlefill;
	float *rowalpha;
	unsigned int vertcount;

public:
	ShellTitleTemplate(void);
	~ShellTitleTemplate();

	// configure
	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

class ShellTitle : public Overlay
{
	int cols;
	int rows;
	unsigned short *titlefill;
	float *rowalpha;
	unsigned int vertcount;

public:
	// constructor
	ShellTitle(const ShellTitleTemplate &aTemplate, unsigned int aId);

	// destructor
	~ShellTitle();

	// draw title
	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};
