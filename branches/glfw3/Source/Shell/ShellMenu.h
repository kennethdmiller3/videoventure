#pragma once

struct ShellMenuItem;

// shell menu page
struct ShellMenuPage
{
	ShellMenuItem *mOption;
	unsigned int mCount;

	//fastdelegate::FastDelegate<void ()> mEnter;
	void (*mEnter)();
	//fastdelegate::FastDelegate<void ()> mExit;
	void (*mExit)();

	ShellMenuPage *mParent;
};

// shell menu
struct ShellMenu
{
	ShellMenuPage *mActive;

	void Push(ShellMenuPage *aPage)
	{
		aPage->mParent = mActive;
		if (mActive && mActive->mExit)
			(mActive->mExit)();
		mActive = aPage;
		if (mActive && mActive->mEnter)
			(mActive->mEnter)();
	}
	void Pop()
	{
		if (mActive && mActive->mExit)
			(mActive->mExit)();
		mActive = mActive->mParent;
		if (mActive && mActive->mEnter)
			(mActive->mEnter)();
	}
	void RenderOptions(unsigned int aId, float aTime, const Transform2 &aTransform);
};

extern ShellMenu shellmenu;
