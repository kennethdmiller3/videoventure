#pragma once
#include "Entity.h"
#include "Renderable.h"
#include "Sprite.h"

class Cloud :
	public Entity, public Renderable
{
public:
	Cloud(unsigned int aId = 0, unsigned int aParentId = 0);
	~Cloud(void);

	// initialize
	void Init(int aCount);

	// render
	virtual void Render(void);
};
