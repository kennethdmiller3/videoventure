#include "StdAfx.h"

#include "Console.h"
#include "Font.h"

// number of frames to transition between hidden and visible states
static const int SLIDE_STEPS = 30;

// default number of lines of history
static const int DEFAULT_MAX_LINES = 256;

// constructor
Console::Console(void(*cbfun)(const char *cmd))
{
	// size the console
	lines = NULL;
	Resize();

	// clear user command line
	inputLineLength = 0;
	inputCursorPos = 0;
	inputLine[0] = '\0';

	// clear history lines
	memset(history, 0, MAX_INPUT_LENGTH * MAX_HISTORY_COUNT);
	historyQueueIndex = 0;
	historyScrollIndex = -1;

	// set default key callback
	// TO DO: pass as constructor parameter?
	commandCallback = cbfun;

	// start hidden
	visibility = 0;

	// debug output
	Print("Console display: %i x %i\n", textWidth, textHeight);
	Print("Console input length: %i\n", MAX_INPUT_LENGTH);
}

// destructor
Console::~Console()
{
	// release the line buffer
	free(lines);
}

// default enter key callback (does nothing)
void Console::DefaultCommandCallback(const char *cmd)
{
}

// resize the console to the current viewport
void Console::Resize()
{
	// get character dimensions in pixels
	characterWidth = (float)FontGetWidth(sDefaultFontHandle, 'A');
	characterHeight = (float)FontGetHeight(sDefaultFontHandle);

	// get viewport dimensions in characters
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	textWidth = xs_FloorToInt(viewport[2]/characterWidth);
	textHeight = xs_FloorToInt(viewport[3]/characterHeight);

	// generate projection matrix
	// (flip vertically so zero is at the top)
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glOrtho(viewport[0], viewport[2], viewport[1], viewport[3], -1, 1);
	glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);
	glPopMatrix();

	// generate model/view matrix
	// (shift to center the console within the viewport)
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(
		0.5f*((float)viewport[2] - textWidth * characterWidth),
		0.5f*((float)viewport[3] - textHeight * characterHeight),
		0
		);
	glGetFloatv(GL_MODELVIEW_MATRIX, modelviewMatrix);
	glPopMatrix();

	// set total number of screen lines
	maxLines = DEFAULT_MAX_LINES;
	
	// reallocate lines
	if (lines)
		free(lines);
	lines = (char *)calloc(maxLines * (textWidth + 1), 1);

	// queued newline
	outputNewline = 0;

	// next output character position
	// (start of the buffer)
	outputCursor = lines;

	// next output line index
	// (first line in the buffer)
	lineQueueIndex = 0;

	// first visible line index
	// (place first line at the bottom of the screen)
	lineScrollIndex = maxLines - textHeight + 1;
}

/* This function draws a single specific console; if you only use one console in
* your program, use Draw() instead */
void Console::Render()
{
	// do nothing if hidden
	if (visibility == 0)
		return;

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	// load the projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(projectionMatrix);

	// load the model/view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(modelviewMatrix);

	// turn off depth test
	glDisable(GL_DEPTH_TEST);

	// disable polygon smoothing
	glDisable(GL_POLYGON_SMOOTH);

    /* Render hiding / showing console in a special manner. Zero means hidden. 1
     * means visible. All other values are traveling toward zero or one. TODO:
     * Make this time dependent */
    if (visibility != 1)
    {
        int v = visibility;

        /* Count down in both directions */
        if (v < 0)
        {
            v ^= -1;
            visibility++;
        }
        else
        {
            v = SLIDE_STEPS - v;
            visibility--;
        }

        float d = textHeight * characterHeight * (1.0f - v * (1.0f / SLIDE_STEPS));
        glTranslatef(0, d, 0);
    }

	// draw the background:
	// untextured translucent black rectangle
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(textWidth * characterWidth, 0);
	glVertex2f(textWidth * characterWidth, textHeight * characterHeight);
	glVertex2f(0, textHeight * characterHeight);
	glEnd();

	// start rendering text
	FontDrawBegin(sDefaultFontHandle);

	// display text: green
	FontDrawColor(Color4(0.0f, 1.0f, 0.0f, 1.0f));

	// start at the first visible index
	int tLine = lineScrollIndex;

	// for each line of the display...
	for (int gLine = 0; gLine < textHeight; gLine++)
	{
		// draw the visible line
		FontDrawString(
			lines + (tLine * textWidth),
			0,
			(textHeight - gLine) * characterHeight,
			characterWidth,
			characterHeight,
			0);

		// advance to the next line
		if (++tLine >= maxLines)
			tLine = 0;
	}

	// if showing a history line...
	if (historyScrollIndex >= 0)
	{
		// draw the specified line in red
		FontDrawColor(Color4(1.0f, 0.0f, 0.0f, 1.0f));
		FontDrawString(
			history[historyScrollIndex],
			0, 0,
			characterWidth,
			characterHeight,
			0);
	}
	else
	{
		// draw input line in light blue 
		FontDrawColor(Color4(0.0f, 0.5f, 1.0f, 1.0f));
		FontDrawString(
			inputLine,
			0, 0,
			characterWidth,
			characterHeight,
			0);

		// draw a cursor in white
		FontDrawColor(Color4(1.0f, 1.0f, 1.0f, 1.0f));
		FontDrawCharacter(
			'_',
			inputCursorPos * characterWidth,
			0,
			characterWidth,
			characterHeight,
			0);
	}

	// finish drawing text
	FontDrawEnd();

	// restore projection matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// restore model/view matrix
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// restore attributes
	glPopAttrib();
}

// formatted print
void Console::Print(const char *s, ...)
{
	// generate output text
	va_list argument;
	char buffer[4096];
	va_start(argument, s);
	vsnprintf(buffer, 4096, s, argument);
	va_end(argument);

	// for each character in the buffer...
	for (char *s = buffer; *s; ++s)
	{
		// if there is a queued newline,
		// or the output position reaches the right side...
		if ((outputNewline) ||
			(outputCursor - (lines + lineQueueIndex * textWidth)) >= (textWidth - 1))
		{
			// clear any queued newline
			outputNewline = 0;

			// advance to the next line
			if (++lineQueueIndex >= maxLines)
				lineQueueIndex = 0;

			// also scroll to the next line
			// TO DO: stay in place if scrolled away from the end
			if (++lineScrollIndex >= maxLines)
				lineScrollIndex = 0;

			// snap to the beginning of line
			outputCursor = lines + lineQueueIndex * textWidth;
		}

		// if the current character is a newline...
		if (*s == '\n')
		{
			// queue the newline
			outputNewline = 1;
			continue;
		}

		// if the current character is a tab...
		if (*s == '\t')
		{
			static const int TAB_WIDTH = 4;
			int count = TAB_WIDTH - (outputCursor - (lines + lineQueueIndex * textWidth)) % TAB_WIDTH;
			while (count--)
				*outputCursor += ' ';
			continue;
		}

		// copy the character
		*outputCursor++ = *s;
	}

	// if not at the end of a line...
	if (outputCursor != lines + (lineQueueIndex + 1) * textWidth - 1)
	{
		// null-terminate the line
		*outputCursor = '\0';
	}
}

// add a line to the history buffer
void Console::AddHistory(char *s)
{
	// advance to the next history line
	if (++historyQueueIndex > MAX_HISTORY_COUNT)
		historyQueueIndex = 0;

	// copy the line
	strncpy(history[historyQueueIndex], s, MAX_INPUT_LENGTH);
}

// pull a line from history into input
void Console::YankHistory()
{
	// if looking back in the history buffer...
	if (historyScrollIndex != -1)
	{
		// copy the selected line into the input buffer
		strncpy(inputLine, history[historyScrollIndex], MAX_INPUT_LENGTH);

		// move cursor to the end of the line
		inputCursorPos = inputLineLength = strlen(inputLine);

		// go back to input mode
		historyScrollIndex = -1;
	}
}

// respond to key and character events
#ifdef USE_SDL
#include "SDL.h"
#define KEY_BACKSPACE   SDLK_BACKSPACE
#define KEY_DELETE      SDLK_DELETE
#define KEY_RETURN      SDLK_RETURN
#define KEY_UP          SDLK_UP
#define KEY_DOWN        SDLK_DOWN
#define KEY_LEFT        SDLK_LEFT
#define KEY_RIGHT       SDLK_RIGHT
#define KEY_PAGEUP      SDLK_PAGEUP
#define KEY_PAGEDOWN    SDLK_PAGEDOWN
#endif
#ifdef USE_GLFW
#include <GL/glfw.h>
#define KEY_BACKSPACE   GLFW_KEY_BACKSPACE
#define KEY_DELETE      GLFW_KEY_DEL
#define KEY_RETURN      GLFW_KEY_ENTER
#define KEY_UP          GLFW_KEY_UP
#define KEY_DOWN        GLFW_KEY_DOWN
#define KEY_LEFT        GLFW_KEY_LEFT
#define KEY_RIGHT       GLFW_KEY_RIGHT
#define KEY_PAGEUP      GLFW_KEY_PAGEUP
#define KEY_PAGEDOWN    GLFW_KEY_PAGEDOWN
#define KMOD_LSHIFT     1
#define KMOD_RSHIFT     2
#define KMOD_SHIFT      (KMOD_LSHIFT|KMOD_RSHIFT)
#endif
#ifdef USE_SFML
#error TO DO
#endif

// key event
int Console::KeyEvent(int sym, int mod)
{
	// if not visible...
	if (visibility < 1)
	{
		// if show/hide key pressed...
		if (sym == '`')
		{  
			// set to slide in
			visibility += SLIDE_STEPS;
			return 1;
		}

		// no action
		return 0;
	}

	switch(sym)
	{
	case '`':

		// set to slide out
		visibility -= SLIDE_STEPS;

		return 1;

	case KEY_DELETE:
	case KEY_BACKSPACE:

		// yank command history if necessary
		YankHistory();

		// if the input buffer is not empty...
		if (inputLineLength)
		{
			// if backspace...
			if (sym == KEY_BACKSPACE)
			{
				// back up the cursor position
				inputCursorPos--;
			}

			// else if at the end of the line...
			else if (inputCursorPos == inputLineLength)
			{
				// nothing to delete
				return 1;
			}

			// move the rest of the line back one character
			char *c   = inputLine +   inputCursorPos;
			char *end = inputLine + --inputLineLength;

			while (c <= end)
			{
				*c = *(c+1);
				c++;
			}
		}

		return 1;

	case KEY_RETURN:

		// yank command history if necessary
		YankHistory();

		// add input buffer to history
		AddHistory(inputLine);

		// display input buffer
		Print("%s\n", inputLine);

		// trigger command callback
		commandCallback(inputLine);

		// clear the input buffer
		inputCursorPos = 0;
		inputLineLength = 0;
		inputLine[0] = '\0';

		return 0;

	case KEY_PAGEUP:

		// scroll up half a page
		lineScrollIndex -= textHeight / 2;

		// wrap around if necessary
		if (lineScrollIndex < 0)
			lineScrollIndex += maxLines;

		printf("scroll index = %i\n", lineScrollIndex);

		return 1;

	case KEY_PAGEDOWN:

		// scroll down half a page
		lineScrollIndex += textHeight / 2;

		// wrap around if necessary
		if (lineScrollIndex >= maxLines)
			lineScrollIndex -= maxLines;

		printf("scroll index = %i\n", lineScrollIndex);

		return 1;

	case KEY_UP:

		// if holding shift...
		if (mod & (KMOD_LSHIFT|KMOD_RSHIFT))
		{
			// scroll up a line
			if (--lineScrollIndex < 0)
				lineScrollIndex = maxLines-1;
		}
		else
		{
			// if not looking at history...
			if (historyScrollIndex == -1)
			{
				// start at the most recent history
				historyScrollIndex = historyQueueIndex;
			}
			else
			{
				// go to the previous history index
				if (--historyScrollIndex < 0)
					historyScrollIndex = MAX_HISTORY_COUNT;
			}

			// if back at the If we've returned to our current position in the command
			// history, we'll just drop out of history mode
			if (historyScrollIndex == historyQueueIndex + 1)
			{
				historyScrollIndex = -1;
			}
		}

		return 1;

	// Arrow key down
	case KEY_DOWN:

		// Shift key is for scrolling the output display
		if (mod & (KMOD_LSHIFT|KMOD_RSHIFT))
		{
			if (++lineScrollIndex >= maxLines)
				lineScrollIndex = 0;
		}

		// No shift key is for scrolling through command history
		else
		{
			// -1 means we aren't look at history yet
			if (historyScrollIndex != -1)
			{
				// Wrap our history scrolling
				if (++historyScrollIndex >= MAX_HISTORY_COUNT)
					historyScrollIndex = 0;

				// If we've returned to our current position in the command
				// history, we'll just drop out of history mode
				if (historyScrollIndex ==
					historyQueueIndex +1)
					historyScrollIndex = -1;
			}
			else
			{
				// TODO: be like, no bitch, there's no history down there
			}
		}

		return 1;

	// Arrow key left
	case KEY_LEFT:

		/* Yank the command history if necessary */
		YankHistory();

		if (inputCursorPos > 0)
			inputCursorPos--;

		return 1;


	// Arrow key right
	case KEY_RIGHT:

		/* Yank the command history if necessary */
		YankHistory();

		if (inputCursorPos <
			inputLineLength)
			inputCursorPos++;

		return 1;

	default:
		return 0;
	}
}

int Console::CharEvent(int unicode)
{
	/* If the terminal is hidden we only check for show/hide key */
	if (visibility < 1)
	{
		return 0;
	}

	if (unicode != 0)
	{
		char *c, *d;

		/* Yank the command history if necessary */
		YankHistory();

		/* Point to the cursor position and the end of the string */
		c = inputLine + inputCursorPos;
		d = inputLine + inputLineLength + 1;

		/* Slide some of the string to the right */
		for (; d != c; d--)
			*d = *(d-1);

		/* Insert new character */
		*c = (char)unicode;

		/* Increment input line length counter */
		inputLineLength++;

		/* Advance input cursor position */
		inputCursorPos++;

		return 1;
	}

	return 0;
}
