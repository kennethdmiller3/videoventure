#pragma once

// TREE NODE
// usage: derive T from this type (e.g. "class T : public TreeNode<T> { ... };")
template <typename T> struct TreeNode
{
	T *mParent;
	T *mFirstChild;
	T *mLastChild;
	T *mPrevSibling;
	T *mNextSibling;

	TreeNode()
		: mParent(NULL)
		, mFirstChild(NULL)
		, mLastChild(NULL)
		, mPrevSibling(NULL)
		, mNextSibling(NULL)
	{
	}

	virtual ~TreeNode()
	{
		Detach();
		while (mFirstChild)
			delete mFirstChild;
	}

	void AttachFirst(T *aParent)
	{
		if (mParent)
			Detach();
		mParent = aParent;
		mPrevSibling = NULL;
		mNextSibling = mParent->mFirstChild;
		if (mParent->mFirstChild)
			mParent->mFirstChild->mPrevSibling = static_cast<T *>(this);
		mParent->mFirstChild = static_cast<T *>(this);
		if (!mParent->mLastChild)
			mParent->mLastChild = static_cast<T *>(this);
	}

	void AttachLast(T *aParent)
	{
		if (mParent)
			Detach();
		mParent = aParent;
		mPrevSibling = mParent->mFirstChild;
		mNextSibling = NULL;
		if (mParent->mLastChild)
			mParent->mLastChild->mNextSibling = static_cast<T *>(this);
		mParent->mLastChild = static_cast<T *>(this);
		if (!mParent->mFirstChild)
			mParent->mFirstChild = static_cast<T *>(this);
	}

	void Detach()
	{
		if (!mParent)
			return;

		if (mParent->mFirstChild == this)
			mParent->mFirstChild = mNextSibling;
		if (mParent->mLastChild == this)
			mParent->mLastChild = mPrevSibling;
		mParent = NULL;
		if (mPrevSibling)
			mPrevSibling->mNextSibling = mNextSibling;
		mPrevSibling = NULL;
		if (mNextSibling)
			mNextSibling->mPrevSibling = mPrevSibling;
		mNextSibling = NULL;
	}
};
