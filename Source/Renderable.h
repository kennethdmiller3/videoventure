#pragma once

class RenderableTemplate
{
public:
	// drawlist buffer
	std::vector<unsigned int> mBuffer;

	// bounding radius
	float mRadius;

	// period
	float mPeriod;

public:
	RenderableTemplate(void);
	~RenderableTemplate();

	// configure
	bool Configure(const TiXmlElement *element);
};

class Renderable
{
private:
	// list of all renderables
	static Renderable *sHead;
	static Renderable *sTail;

	// identifier
	unsigned int id;

	// linked list
	Renderable *mNext;
	Renderable *mPrev;

	// show?
	bool show;

	// bounding radius
	float mRadius;

protected:
	// time offset
	static unsigned int sTurn;
	static float sOffset;

	// creation turn
	unsigned int mStart;
	float mFraction;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Renderable(void);
	Renderable(const RenderableTemplate &aTemplate, unsigned int aId);
	~Renderable(void);

	// visibility
	void Show(void);
	void Hide(void);

	// set turn
	static void SetTurn(unsigned int aTurn)
	{
		sTurn = aTurn;
	}
	void SetFraction(float aFraction)
	{
		mFraction = aFraction;
	}

	// render
	static void RenderAll(float aRatio, float aStep, const AlignedBox2 &aView);
	void Render(float aStep);
};

namespace Database
{
	extern Typed<RenderableTemplate> renderabletemplate;
	extern Typed<Renderable *> renderable;
}
