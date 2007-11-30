#pragma once

class Renderable
{
private:
	// list of all renderables
	typedef std::list<Renderable *> List;
	static List sAll;

	// list entry
	List::iterator entry;

public:
	// draw list
	GLuint mDraw;

public:
	Renderable(void);
	~Renderable(void);

	// configure
	virtual bool Configure(TiXmlElement *element);

	// visibility
	void Show(void);
	void Hide(void);

	// render
	static void RenderAll(void);
	virtual void Render(void) = 0;
};
