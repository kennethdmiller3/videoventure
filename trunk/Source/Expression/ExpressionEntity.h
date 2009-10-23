#pragma once

#include "Expression.h"

// entity context
// (extends expression context)
struct EntityContext : public Expression::Context
{
	const unsigned int *mBegin;
	const unsigned int *mEnd;
	float mParam;
	unsigned int mId;
	Database::Typed<float> *mVars;
	bool mOwned;

	GAME_API EntityContext(const unsigned int *aBuffer, const size_t aSize, float aParam, unsigned int aId, Database::Typed<float> *aVars = NULL);
	GAME_API ~EntityContext();

	void Restart(void)
	{
		mStream = mBegin;
	}
	bool IsDone(void) const
	{
		return mStream >= mEnd;
	}
};

namespace Expression
{
	// entity transform
	GAME_API __m128 EvaluatePosition(EntityContext &aContext);
	GAME_API __m128 EvaluateVelocity(EntityContext &aContext);
}
