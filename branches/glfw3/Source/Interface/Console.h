#pragma once

class Console
{
public:
	// maximum length of user input
	static const int MAX_INPUT_LENGTH = 256;

	// number of user input lines to remember
	static const int MAX_HISTORY_COUNT = 32;

	// projection matrix
	GLfloat projectionMatrix[16];

	// model/view matrix
	GLfloat modelviewMatrix[16];

	// display buffer
	char *lines;
	int maxLines;
	int lineQueueIndex;
	int lineScrollIndex;

	// history buffer
	char history[MAX_HISTORY_COUNT][MAX_INPUT_LENGTH];
	int historyQueueIndex;
	int historyScrollIndex;

	// input buffer
	char inputLine[MAX_INPUT_LENGTH];
	int inputCursorPos, inputLineLength;

	// visible area in characters
	int textWidth;
	int textHeight;

	// output position
	char *outputCursor;
	int outputNewline;

	// character size in GL units
	GLfloat characterWidth;
	GLfloat characterHeight;

	// visibility state
	int visibility;

	// command callback
	void(*commandCallback)(const char *cmd);

public:
	Console(void(*cbfun)(const char *cmd) = DefaultCommandCallback);
	~Console();

	static void DefaultCommandCallback(const char *cmd);

	void Resize();

	void SetVisibility(int visible)
	{
		visibility = visible;
	}

	int GetVisibility()
	{
		return visibility;
	}

	void Render();

	void Print(const char *s, ...);

	void AddHistory(char *s);
	void YankHistory();

	int KeyEvent(int key, int mod);
	int CharEvent(int unicode);
};
