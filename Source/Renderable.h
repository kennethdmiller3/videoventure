#pragma once

class RenderableTemplate
{
public:
	// draw list
	GLuint mDraw;

public:
	RenderableTemplate(void);
	virtual ~RenderableTemplate();

	// configure
	virtual bool Configure(TiXmlElement *element);
};

class Renderable : public RenderableTemplate
{
private:
	// list of all renderables
	typedef std::list<Renderable *> List;
	static List sAll;

	// list entry
	List::iterator entry;
	bool show;

public:
	Renderable(void);
	Renderable(const RenderableTemplate &aTemplate);
	~Renderable(void);

	// visibility
	void Show(void);
	void Hide(void);

	// render
	static void RenderAll(void);
	virtual void Render(void);
};

namespace Database
{
	extern Typed<RenderableTemplate> renderabletemplate;
	extern Typed<Renderable> renderable;
	extern Typed<GLuint> drawlist;
}
