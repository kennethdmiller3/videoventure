/* oglconsole -- gpl license here */

#ifndef _OGLCONSOLE_H
#define _OGLCONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque to you you lowly user */
typedef struct _OGLCONSOLE_Console *OGLCONSOLE_Console;

/* Initialize/uninitialize OGLConsole */
OGLCONSOLE_Console OGLCONSOLE_Create();
void OGLCONSOLE_Destroy(OGLCONSOLE_Console console);
void OGLCONSOLE_Quit();

/* Set console which has PROGRAMMER focus (not application focus) */

/* This function renders the console */
void OGLCONSOLE_Draw();
void OGLCONSOLE_Render(OGLCONSOLE_Console console);

/* Print to the console */
void OGLCONSOLE_Print(char *s, ...);
void OGLCONSOLE_Output(OGLCONSOLE_Console console, const char *s, ...);

/* Register a callback with the console */
void OGLCONSOLE_EnterKey(void(*cbfun)(OGLCONSOLE_Console console, char *cmd));

/* This function tries to handle the incoming SDL event. In the future there may
 * be non-SDL analogs for input systems such as GLUT. Returns true if the event
 * was handled by the console. If console is hidden, no events are handled. */
#if defined(OGLCONSOLE_USE_SDL)
#include "SDL.h"
int OGLCONSOLE_SDLEvent(SDL_Event * e);
#endif

/* Sets the current console for receiving user input */
void OGLCONSOLE_FocusConsole(OGLCONSOLE_Console console);

/* Sets the current console for making options changes to */
void OGLCONSOLE_EditConsole(OGLCONSOLE_Console console);

/* Sets the dimensions of the console in lines and columns of characters. */
void OGLCONSOLE_SetDimensions(int width, int height);

/* Show or hide the console. */
void OGLCONSOLE_SetVisibility(int visible);

#ifdef __cplusplus
}
#endif

#endif

