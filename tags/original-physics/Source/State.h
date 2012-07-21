#pragma once

#include "Updatable.h"

namespace Expression
{
	struct Context;
}

class StateTemplate
{
public:
	unsigned int mSubId;
	unsigned int mStateId;
	std::vector<unsigned int> mEnter;
	std::vector<unsigned int> mExit;

public:
	StateTemplate(void);
	~StateTemplate(void);

	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId, unsigned int aSubId);

	void Enter(unsigned int aId) const;
	void Exit(unsigned int aId) const;
};

class TransitionTemplate
{
public:
	unsigned int mStateId;
	unsigned int mTargetId;
	std::vector<unsigned int> mGuard;
	std::vector<unsigned int> mAction;

public:
	TransitionTemplate(void);
	~TransitionTemplate(void);

	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId, unsigned int aSubId);

	bool EvaluateGuard(unsigned int aId) const;
};

class StateMachine : public Updatable	// <-- temporary
{
public:
	unsigned int mActiveId;

public:
	StateMachine(unsigned int aId);
	~StateMachine(void);

	void Update(float aStep);	// <-- temporary

	void EnterState(unsigned int aStateId);
	void ExitState(unsigned int aStateId);
};

