#pragma once
#include "Entity.h"
#include "Renderable.h"
#include "Sprite.h"

class Cloud :
	public Entity, public Renderable
{
public:
	Cloud(int aCount);
	~Cloud(void);

	// render
	virtual void Render(void);
};
