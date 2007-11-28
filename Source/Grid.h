#pragma once
#include "Entity.h"
#include "Renderable.h"

class Grid :
	public Entity, public Renderable
{
public:
	Grid(void);
	~Grid(void);

	// render
	void Render(void);
};
