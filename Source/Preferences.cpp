#include "StdAfx.h"

// read preferences
bool ReadPreferences(const char *config)
{
	// load input binding file
	DebugPrint("Preferences %s\n", config);
	TiXmlDocument document(config);
	if (!document.LoadFile())
		DebugPrint("error loading preferences file \"%s\": %s\n", config, document.ErrorDesc());

	// process child elements of the root
	if (const TiXmlElement *root = document.FirstChildElement("preferences"))
	{
		for (const TiXmlElement *element = root->FirstChildElement(); element != NULL; element = element->NextSiblingElement())
		{
			switch (Hash(element->Value()))
			{
			case 0x1d215c8f /* "resolution" */:
				element->QueryIntAttribute("width", &SCREEN_WIDTH);
				element->QueryIntAttribute("height", &SCREEN_HEIGHT);
				break;

			case 0x5032fb58 /* "fullscreen" */:
				{
					int enable = SCREEN_FULLSCREEN;
					element->QueryIntAttribute("enable", &enable);
					SCREEN_FULLSCREEN = enable != 0;
				}
				break;

			case 0x423e6b0c /* "verticalsync" */:
				{
					int enable = OPENGL_SWAPCONTROL;
					element->QueryIntAttribute("enable", &enable);
					OPENGL_SWAPCONTROL = enable != 0;
				}
				break;

			case 0x47d0f228 /* "multisample" */:
				element->QueryIntAttribute("samples", &OPENGL_MULTISAMPLE);
				break;

			case 0xf744f3b2 /* "motionblur" */:
				element->QueryIntAttribute("steps", &MOTIONBLUR_STEPS);
				if (element->QueryFloatAttribute("strength", &MOTIONBLUR_TIME) == TIXML_SUCCESS)
					MOTIONBLUR_TIME /= 6000;
				break;

			case 0x0e0d9594 /* "sound" */:
				element->QueryIntAttribute("channels", &SOUND_CHANNELS);
				if (element->QueryFloatAttribute("volume", &SOUND_VOLUME) == TIXML_SUCCESS)
					SOUND_VOLUME /= 100;
				break;
			}
		}
		return true;
	}

	return false;
}

// write preferences
bool WritePreferences(const char *config)
{
	// load input binding file
	DebugPrint("Preferences %s\n", config);
	TiXmlDocument document(config);

	TiXmlDeclaration * declaration = new TiXmlDeclaration( "1.0", "", "" );
	document.LinkEndChild(declaration);

	TiXmlElement *preferences = new TiXmlElement("preferences");
	document.LinkEndChild(preferences);

	TiXmlElement *resolution = new TiXmlElement("resolution");
	resolution->SetAttribute("width", SCREEN_WIDTH);
	resolution->SetAttribute("height", SCREEN_HEIGHT);
	preferences->LinkEndChild(resolution);

	TiXmlElement *fullscreen = new TiXmlElement("fullscreen");
	fullscreen->SetAttribute("enable", SCREEN_FULLSCREEN);
	preferences->LinkEndChild(fullscreen);

	TiXmlElement *verticalsync = new TiXmlElement("verticalsync");
	verticalsync->SetAttribute("enable", OPENGL_SWAPCONTROL);
	preferences->LinkEndChild(verticalsync);

	TiXmlElement *multisample = new TiXmlElement("multisample");
	multisample->SetAttribute("samples", OPENGL_MULTISAMPLE);
	preferences->LinkEndChild(multisample);

	TiXmlElement *motionblur = new TiXmlElement("motionblur");
	motionblur->SetAttribute("steps", MOTIONBLUR_STEPS);
	motionblur->SetAttribute("strength", xs_RoundToInt(MOTIONBLUR_TIME*6000));
	preferences->LinkEndChild(motionblur);

	TiXmlElement *sound = new TiXmlElement("sound");
	sound->SetAttribute("channels", SOUND_CHANNELS);
	sound->SetAttribute("volume", xs_RoundToInt(SOUND_VOLUME*100));
	preferences->LinkEndChild(sound);

	document.SaveFile();
	return true;
}
