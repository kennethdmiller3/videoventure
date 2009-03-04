#pragma once

#include "TreeNode.h"

class OverlayTemplate
{
public:
	// period
	float mPeriod;

public:
	OverlayTemplate(void);
	~OverlayTemplate();

	// configure
	bool Configure(const TiXmlElement *element, unsigned int aId);
};

class Overlay : public TreeNode<Overlay>
{
public:
	typedef fastdelegate::FastDelegate<void (unsigned int, float, const Transform2 &)> Action;

protected:
	// identifier
	unsigned int mId;

private:
	// root overlay
	static Overlay sRoot;
	bool mActive;

	// render action
	Action mAction;

protected:
	// creation turn
	unsigned int mStart;
	float mFraction;

public:
	Overlay(unsigned int aId);
	virtual ~Overlay(void);

	// set action
	void SetAction(Action aAction)
	{
		mAction = aAction;
	}

	// visibility
	void Show(void);
	void Hide(void);

	// render
	static void RenderAll(Overlay &aRoot = sRoot);
};



namespace Database
{
	extern Typed<OverlayTemplate> overlaytemplate;
	extern Typed<Overlay *> overlay;
}
