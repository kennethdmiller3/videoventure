#include "StdAfx.h"

namespace Platform
{
	bool Init(void)
	{
		// initialize the window
		InitWindow();

		// hide the mouse cursor
		window.ShowMouseCursor(false);

		// success!
		return true;
	}

	void Done(void)
	{
	}
}
