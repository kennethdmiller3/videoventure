#include "StdAfx.h"

#include "State.h"
#include "ExpressionConfigure.h"
#include "ExpressionAction.h"
#include "Entity.h"
#include "Link.h"

//#define DEBUG_STATE_MACHINE

namespace Database
{
	Typed<Typed<StateTemplate> > statetemplate(0xeb613ee8 /* "statetemplate" */);
	Typed<Typed<TransitionTemplate> > transitiontemplate(0xaeec4ec4 /* "transitiontemplate" */);
	Typed<StateMachine *> statemachine(0x77f310ef /* "statemachine" */);
	Typed<unsigned int> stateturn(0xce13a8fd /* "stateturn" */);

	namespace Loader
	{
		class StateLoader
		{
		public:
			StateLoader()
			{
				AddConfigure(0x783132f6 /* "state" */, Entry(this, &StateLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				Database::Typed<StateTemplate> &states = Database::statetemplate.Open(aId);
				unsigned int aSubId = Hash(element->Attribute("name"));
				std::string name = Database::name.Get(aId);
				name += ".";
				name += element->Attribute("name");
				Database::name.Put(Hash(name.c_str()), name);
				StateTemplate &state = states.Open(aSubId);
				state.Configure(element, aId, aSubId);
				states.Close(aSubId);
				Database::statetemplate.Close(aId);
			}
		}
		stateloader;

		class TransitionLoader
		{
		public:
			TransitionLoader()
			{
				AddConfigure(0xa62782e2 /* "transition" */, Entry(this, &TransitionLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				Database::Typed<TransitionTemplate> &transitions = Database::transitiontemplate.Open(aId);
				unsigned int aSubId = Hash(element->Attribute("name"));
				TransitionTemplate &transition = transitions.Open(aSubId);
				transition.Configure(element, aId, aSubId);
				transitions.Close(aSubId);
				Database::transitiontemplate.Close(aId);
			}
		}
		transitionloader;
	}

	namespace Initializer
	{
		class StateInitializer
		{
		public:
			StateInitializer()
			{
				AddActivate(0xeb613ee8 /* "statetemplate" */, Entry(this, &StateInitializer::Activate));
				AddDeactivate(0xeb613ee8 /* "statetemplate" */, Entry(this, &StateInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				if (StateMachine *machine = Database::statemachine.Get(aId))
					return;
				StateMachine *machine = new StateMachine(aId);
				Database::statemachine.Put(aId, machine);
			}

			void Deactivate(unsigned int aId)
			{
				if (StateMachine *machine = Database::statemachine.Get(aId))
				{
					delete machine;
					Database::statemachine.Delete(aId);
				}
			}
		}
		stateinitializer;
	}
}

//
// STATE TEMPLATE
//

StateTemplate::StateTemplate(void)
: mSubId(0)
, mStateId(0)
{
}

StateTemplate::~StateTemplate(void)
{
}

bool StateTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId, unsigned int aSubId)
{
	// get the state name
	const char *name = element->Attribute("name");

	// save sub id
	mSubId = aSubId;

	// generate a state id from the parent id and state name 
	mStateId = Hash(name, Hash(".", aId));

	// save state name
	std::string &namestring = Database::name.Open(mStateId);
	namestring = Database::name.Get(aId);
	namestring.append(".");
	namestring.append(name);
	Database::name.Close(mStateId);

	// if the state is inheriting properties...
	if (const char *inherit = element->Attribute("inherit"))
	{
		// get parent identifier
		unsigned int aParentId = Hash(inherit);

		if (aParentId && !Database::name.Find(aParentId))
			DebugPrint("warning: template \"%s\" parent \"%s\" not found\n", element->Attribute("name"), inherit);

		// inherit parent components
		Database::Inherit(mStateId, aParentId);
	}

	// for each child tag...
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *value = child->Value();

		switch (Hash(value))
		{
		case 0xddfde10d /* "enter" */:
			{
				ConfigureAction(child, mEnter);
			}
			break;

		case 0xcded1a85 /* "exit" */:
			{
				ConfigureAction(child, mExit);
			}
			break;

		default:
			{
				// process world item
				const Database::Loader::Entry &configure = Database::Loader::GetConfigure(Hash(value));
				if (configure)
					configure(mStateId, child);
				else
					DebugPrint("state \"%s\" skipping item \"%s\"\n", namestring.c_str(), value);
			}
			break;
		}
	}

	return true;
}

void StateTemplate::Enter(unsigned int aId) const
{
#ifdef DEBUG_STATE_MACHINE
	DebugPrint("Entering \"%s\" id=%08x\n", Database::name.Get(mStateId).c_str(), aId);
#endif

	// state instance id
	unsigned int aStateId = aId ^ mStateId;

	// set parent
	Database::parent.Put(aStateId, mStateId);

	// set owner
	Database::owner.Put(aStateId, Database::owner.Get(aId));

	// set creator
	Database::creator.Put(aStateId, aId);

	// add a pseudo-backlink (HACK)
	Database::backlink.Put(aStateId, aId);

	// share the parent entity (HACK)
	Database::entity.Put(aStateId, Database::entity.Get(aId));

	// save state enter time
	Database::stateturn.Put(aStateId, sim_turn);

	// activate the state
	Database::Activate(aStateId);

	// perform enter action
	if (!mEnter.empty())
	{
		EntityContext context(&mEnter[0], mEnter.size(), 0.0f, aStateId);
		while (context.mStream < context.mEnd)
			Expression::Evaluate<void>(context);
	}
}

void StateTemplate::Exit(unsigned int aId) const
{
#ifdef DEBUG_STATE_MACHINE
	DebugPrint("Exiting \"%s\" id=%08x\n", Database::name.Get(mStateId).c_str(), aId);
#endif

	// state instance id
	unsigned int aStateId = aId ^ mStateId;

	// perform exit action
	if (!mExit.empty())
	{
		EntityContext context(&mExit[0], mExit.size(), 0.0f, aStateId);
		while (context.mStream < context.mEnd)
			Expression::Evaluate<void>(context);
	}

	// deactivate the state
	Database::Deactivate(aStateId);

	// release the parent entity
	Database::entity.Delete(aStateId);

	// delete state properties
	Database::Delete(aStateId);
}


//
// TRANSITION TEMPLATE
//

TransitionTemplate::TransitionTemplate(void)
: mStateId(0)
, mTargetId(0)
{
}

TransitionTemplate::~TransitionTemplate(void)
{
}

bool TransitionTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId, unsigned int aSubId)
{
	mStateId = aId;

	if (const char *target = element->Attribute("target"))
		mTargetId = Hash(target);

	if (const tinyxml2::XMLElement *arg1 = element->FirstChildElement())
		Expression::Loader<bool>::Configure(arg1, mGuard, sScalarNames, sScalarDefault);

	return false;
}

bool TransitionTemplate::EvaluateGuard(unsigned int aId) const
{
	if (mGuard.empty())
		return true;

	EntityContext context(&mGuard[0], mGuard.size(), (sim_turn - Database::stateturn.Get(mStateId)) * sim_step, aId ^ mStateId);
	return Expression::Evaluate<bool>(context);
}


//
// STATE MACHINE
//

StateMachine::StateMachine(unsigned int aId)
: Updatable(aId)
, mActiveId(0)
{
	// if the database has a "start" state...
	if (const StateTemplate *state = Database::statetemplate.Get(mId).Find(0x652b04df /* "start" */))
	{
		// start in the start state (naturally)
		mActiveId = 0x652b04df /* "start" */;
		state->Enter(mId);
	}
	else
	{
		// start in the first state
		Database::Typed<StateTemplate>::Iterator itor(Database::statetemplate.Find(mId));
		if (itor.IsValid())
		{
			mActiveId = itor.GetKey();
			itor.GetValue().Enter(mId);
		}
	}

	SetAction(Action(this, &StateMachine::Update));
	Activate();
}

StateMachine::~StateMachine(void)
{
	// exit the current state
	if (const StateTemplate *state = Database::statetemplate.Get(mId).Find(mActiveId))
		state->Exit(mId);
}

void StateMachine::Update(float aStep)
{
	// get current state
	const StateTemplate &curstate = Database::statetemplate.Get(mId).Get(mActiveId);

	// hacktastic!
	for (Database::Typed<TransitionTemplate>::Iterator itor(Database::transitiontemplate.Find(curstate.mStateId)); itor.IsValid(); ++itor)
	{
		if (itor.GetValue().EvaluateGuard(mId))
		{
			curstate.Exit(mId);
			mActiveId = itor.GetValue().mTargetId;
			const StateTemplate &newstate = Database::statetemplate.Get(mId).Get(mActiveId);
			newstate.Enter(mId);
			break;
		}
	}
}
