#include "Platform.h"

namespace Platform
{
	sf::RenderWindow window;

	bool OpenWindow()
	{
		// create the window
		window.Create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32), "Shmup!", SCREEN_FULLSCREEN ? sf::Style::Fullscreen : sf::Style::Close, sf::WindowSettings(32, 0, OPENGL_MULTISAMPLE));
		window.UseVerticalSync(OPENGL_SWAPCONTROL);
	}

	void CloseWindow()
	{
		// close the window
		window.Close();
	}
}
