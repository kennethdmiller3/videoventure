#include "StdAfx.h"

#include "TurnAction.h"

//
// TURN ACTION QUEUE

// turn action
struct TurnAction
{
	unsigned int mTurn;
	float mFraction;
	fastdelegate::FastDelegate<void ()> mAction;

	TurnAction(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction)
		: mTurn(aTurn), mFraction(aFraction), mAction(aAction)
	{
	}
};

// compare turn actions
struct TurnActionCompare
{
	bool operator() (const TurnAction &a1, const TurnAction &a2)
	{
		float delta = (int)(a1.mTurn - a2.mTurn) + a1.mFraction - a2.mFraction;
		return (delta > 0.0f) || (delta == 0.0f && a1.mAction > a2.mAction);
	}
};
TurnActionCompare turnactioncompare;

// queued actions
std::deque<TurnAction> turnactions;

// queue a turn action
void OnTurn(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction)
{
	turnactions.push_back(TurnAction(aTurn, aFraction, aAction));
}

// perform actions for this turn
void DoTurn(void)
{
	while (!turnactions.empty())
	{
		std::make_heap(turnactions.begin(), turnactions.end(), turnactioncompare);
		const TurnAction &a = turnactions.front();

		if (int(sim_turn - a.mTurn) + sim_fraction - a.mFraction < 0.0f)
			break;

		(a.mAction)();
		turnactions.pop_front();
	}
}


