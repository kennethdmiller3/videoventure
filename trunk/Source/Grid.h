#pragma once
#include "Entity.h"
#include "Renderable.h"

class Grid :
	public Entity, public Renderable
{
public:
	Grid(unsigned int aId = 0, unsigned int aParentId = 0);
	~Grid(void);

	// configure
	virtual bool Configure(TiXmlElement *element);

	// render
	virtual void Render(const Matrix2 &transform);
};
