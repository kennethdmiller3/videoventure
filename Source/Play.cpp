#include "StdAfx.h"
#include "GameState.h"
#include "Player.h"
#include "PlayerCamera.h"
#include "PlayerHUD.h"
#include "Input.h"
#include "Sound.h"
#include "Collidable.h"
#include "World.h"
#include "Drawlist.h"
#include "Shell.h"


bool InitInput(const char *config)
{
	// clear existing bindings
	input.Clear();

	// load input binding file
	DebugPrint("Input %s\n", config);
	TiXmlDocument document(config);
	if (!document.LoadFile())
		DebugPrint("error loading input file \"%s\": %s\n", config, document.ErrorDesc());

	// process child elements of the root
	if (const TiXmlElement *root = document.FirstChildElement("input"))
	{
		input.Configure(root);
		return true;
	}

	return false;
}

bool SplitLevel(const char *config, const char *output)
{
	// load level data file
	DebugPrint("Level %s -> %s\n", config, output);
	TiXmlDocument document(config);
	document.SetCondenseWhiteSpace(false);
	if (!document.LoadFile())
		DebugPrint("error loading level file \"%s\": %s\n", config, document.ErrorDesc());

	// if the document has a world section...
	if (const TiXmlElement *root = document.FirstChildElement("world"))
	{
		// output level data file
		TiXmlDocument split(output);
		split.SetCondenseWhiteSpace(false);
		TiXmlElement *splitroot = NULL;

		// copy nodes from the original
		for (const TiXmlNode *node = document.FirstChild(); node != NULL; node = node->NextSibling())
		{
			if (node == root)
			{
				splitroot = new TiXmlElement(root->Value());
				for (const TiXmlAttribute *attribute = root->FirstAttribute(); attribute != NULL; attribute = attribute->Next())
					splitroot->SetAttribute(attribute->Name(), attribute->Value());
				split.LinkEndChild(splitroot);
			}
			else
			{
				split.InsertEndChild(*node);
			}
		}

		// for each node...
		for (const TiXmlNode *node = root->FirstChild(); node != NULL; node = node->NextSibling())
		{
			// if the node is an element...
			if (const TiXmlElement *element = node->ToElement())
			{
				// if the element is not an instance
				if (Hash(element->Value()) != 0xd33ff5da /* "entity" */)
				{
					// if the element has a name...
					if (const char *name = element->Attribute("name"))
					{
						// export child element as a separate XML file
						char filename[256];
						TIXML_SNPRINTF(filename, sizeof(filename), "%s/%s.xml", element->Value(), name);
						DebugPrint("%s\n", filename);
						TiXmlDocument piece(filename);
						TiXmlDeclaration * declaration = new TiXmlDeclaration( "1.0", "", "" );
						piece.LinkEndChild(declaration);
						piece.InsertEndChild(*element);
						if (piece.SaveFile())
						{
							// insert an import element
							TiXmlElement *import = new TiXmlElement("import");
							import->SetAttribute("name", filename);
							splitroot->LinkEndChild(import);

							continue;
						}
					}
				}
			}

			// copy to the output
			splitroot->InsertEndChild(*node);
		}

		split.SaveFile();

		return true;
	}

	return false;
}

bool InitLevel(const char *config)
{
	// load level data file
	DebugPrint("Level %s\n", config);
	TiXmlDocument document(config);
	if (!document.LoadFile())
		DebugPrint("error loading level file \"%s\": %s\n", config, document.ErrorDesc());

	// if the document has a world section...
	if (const TiXmlElement *root = document.FirstChildElement("world"))
	{
		// process the world
		ConfigureWorldItem(root);

		// get the reticule draw list (HACK)
		reticule_handle = Database::drawlist.Get(0x170e4c58 /* "reticule" */);

		// play the startup sound (HACK)
		PlaySoundCue(0x94326baa /* "startup" */);

		return true;
	}

	// clear the reticule draw list (HACK)
	reticule_handle = 0;

	// show the mouse cursor
	Platform::ShowCursor(true);

	return false;

}


// player join
void PlayerJoinListener(unsigned int aId)
{
	// create player camera
	PlayerCamera *playercamera = new PlayerCamera(aId);
	Database::playercamera.Put(aId, playercamera);
	playercamera->Activate();

	// create player hud overlay
	// (creates game-specific components)
	PlayerHUD *playerhud = new PlayerHUD(aId);
	Database::playerhud.Put(aId, playerhud);

	// show the HUD
	playerhud->Show();
}

// player quit
void PlayerQuitListener(unsigned int aId)
{
	if (PlayerHUD *playerhud = Database::playerhud.Get(aId))
	{
		// remove player hud overlay
		delete playerhud;
		Database::playerhud.Delete(aId);
	}

	if (PlayerCamera *playercamera = Database::playercamera.Get(aId))
	{
		// remove player camera
		delete playercamera;
		Database::playercamera.Delete(aId);
	}
}

// enter play state
void EnterPlayState()
{
	// clear the screen
	glClear(
		GL_COLOR_BUFFER_BIT
#ifdef ENABLE_DEPTH_TEST
		| GL_DEPTH_BUFFER_BIT
#endif
		);

	// show back buffer
	Platform::Present();

	// reset camera position
	camerapos[0] = camerapos[1] = Vector2(0, 0);

	// reset simulation timer
	sim_rate = float(SIMULATION_RATE);
	sim_step = 1.0f / sim_rate;
	sim_turn = 0;
	sim_fraction = 1.0f;

	// input binding
	InitInput(INPUT_CONFIG.c_str());

	// add a join listener
	Database::playerjoin.Put(0xe28d61c6 /* "hud" */, PlayerJoinListener);

	// add a quit listener
	Database::playerquit.Put(0xe28d61c6 /* "hud" */, PlayerQuitListener);

	// level configuration
	if (!InitLevel(LEVEL_CONFIG.c_str()))
		setgamestate = STATE_SHELL;

	// start audio
	Sound::Resume();

	// create escape overlay
	Overlay *escape = new Overlay(0x9e212406 /* "escape" */);
	Database::overlay.Put(0x9e212406 /* "escape" */, escape);
	escape->SetAction(Overlay::Action(RenderEscapeOptions));

	// set to runtime mode
	runtime = true;

	DebugPrint("Simulating at %dHz (x%f)\n", SIMULATION_RATE, TIME_SCALE);
}

// run play state
// exit play state
void ExitPlayState()
{
	DebugPrint("Quitting...\n");

	// stop audio
	Sound::Pause();

	// stop any startup sound (HACK)
	StopSoundCue(0x94326baa /* "startup" */);

	// delete escape overlay
	delete Database::overlay.Get(0x9e212406 /* "escape" */);
	Database::overlay.Delete(0x9e212406 /* "escape" */);

	// clear all databases
	Database::Cleanup();

	// collidable done
	Collidable::WorldDone();

	// set to non-runtime mode
	runtime = false;
}
