#pragma once

// TO DO: implement game state machine as an actual state machine

// game state machine (HACK)
enum GameStateType
{
	STATE_NONE,
	STATE_SHELL,
	STATE_PLAY,
	STATE_RELOAD,
	STATE_QUIT,
	NUM_GAME_STATES
};
extern GameStateType curgamestate;
extern GameStateType setgamestate;

extern bool GameStateUpdate(void);