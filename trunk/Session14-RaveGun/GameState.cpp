#include "StdAfx.h"

#include "GameState.h"

// game state machine (HACK)
GameStateType curgamestate = STATE_NONE;
GameStateType setgamestate = STATE_SHELL;

extern void RunState();
extern void EnterShellState();
extern void ExitShellState();
extern void EnterPlayState();
extern void ExitPlayState();
extern void ReloadState();

// state structure
struct GameState
{
	fastdelegate::FastDelegate<void ()> OnEnter;
	fastdelegate::FastDelegate<void ()> OnUpdate;
	fastdelegate::FastDelegate<void ()> OnExit;
};

GameState gamestates[NUM_GAME_STATES] = 
{
	{ NULL, NULL, NULL },
	{ EnterShellState, RunState, ExitShellState },
	{ EnterPlayState, RunState, ExitPlayState },
	{ NULL, ReloadState, NULL },
	{ NULL, NULL, NULL }
};

bool GameStateUpdate(void)
{
	if (setgamestate != curgamestate)
	{
		if (gamestates[curgamestate].OnExit)
			gamestates[curgamestate].OnExit();
		curgamestate = setgamestate;
		if (gamestates[curgamestate].OnEnter)
			gamestates[curgamestate].OnEnter();
	}
	if (gamestates[curgamestate].OnUpdate)
		gamestates[curgamestate].OnUpdate();
	return curgamestate != STATE_QUIT;
}


//
// RELOAD STATE

void ReloadState()
{
	setgamestate = STATE_PLAY;
}
