#pragma once

// queue a turn action
extern void OnTurn(unsigned int aTurn, float aFraction, fastdelegate::FastDelegate<void ()> aAction);

// do actions for this turn
extern void DoTurn(void);