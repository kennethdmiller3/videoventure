#include "StdAfx.h"

// read preferences
bool ReadPreferences(const char *config)
{
	// load input binding file
	DebugPrint("Preferences %s\n", config);
	tinyxml2::XMLDocument document;
	if (document.LoadFile(config) != tinyxml2::XML_SUCCESS)
		DebugPrint("error loading preferences file \"%s\": %s %s\n", config, document.GetErrorStr1(), document.GetErrorStr2());

	// process child elements of the root
	if (const tinyxml2::XMLElement *root = document.FirstChildElement("preferences"))
	{
		for (const tinyxml2::XMLElement *element = root->FirstChildElement(); element != NULL; element = element->NextSiblingElement())
		{
			switch (Hash(element->Value()))
			{
			case 0x1d215c8f /* "resolution" */:
				element->QueryIntAttribute("width", &SCREEN_WIDTH);
				element->QueryIntAttribute("height", &SCREEN_HEIGHT);
				break;

			case 0x5032fb58 /* "fullscreen" */:
				element->QueryBoolAttribute("enable", &SCREEN_FULLSCREEN);
				break;

			case 0x423e6b0c /* "verticalsync" */:
				element->QueryBoolAttribute("enable", &OPENGL_SWAPCONTROL);
				break;

			case 0x47d0f228 /* "multisample" */:
				element->QueryIntAttribute("samples", &OPENGL_MULTISAMPLE);
				break;

			case 0xf744f3b2 /* "motionblur" */:
				element->QueryIntAttribute("steps", &MOTIONBLUR_STEPS);
				if (element->QueryFloatAttribute("strength", &MOTIONBLUR_TIME) == tinyxml2::XML_SUCCESS)
					MOTIONBLUR_TIME /= 6000;
				break;

			case 0x0e0d9594 /* "sound" */:
				element->QueryIntAttribute("channels", &SOUND_CHANNELS);
				if (element->QueryFloatAttribute("volume", &SOUND_VOLUME_EFFECT) == tinyxml2::XML_SUCCESS)
					SOUND_VOLUME_MUSIC = SOUND_VOLUME_EFFECT /= 100;
				if (element->QueryFloatAttribute("effectvolume", &SOUND_VOLUME_EFFECT) == tinyxml2::XML_SUCCESS)
					SOUND_VOLUME_EFFECT /= 100;
				if (element->QueryFloatAttribute("musicvolume", &SOUND_VOLUME_MUSIC) == tinyxml2::XML_SUCCESS)
					SOUND_VOLUME_MUSIC /= 100;
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
	tinyxml2::XMLDocument document;

	tinyxml2::XMLDeclaration * declaration = document.NewDeclaration( "1.0" );
	document.LinkEndChild(declaration);

	tinyxml2::XMLElement *preferences = document.NewElement("preferences");
	document.LinkEndChild(preferences);

	tinyxml2::XMLElement *resolution = document.NewElement("resolution");
	resolution->SetAttribute("width", SCREEN_WIDTH);
	resolution->SetAttribute("height", SCREEN_HEIGHT);
	preferences->LinkEndChild(resolution);

	tinyxml2::XMLElement *fullscreen = document.NewElement("fullscreen");
	fullscreen->SetAttribute("enable", SCREEN_FULLSCREEN);
	preferences->LinkEndChild(fullscreen);

	tinyxml2::XMLElement *verticalsync = document.NewElement("verticalsync");
	verticalsync->SetAttribute("enable", OPENGL_SWAPCONTROL);
	preferences->LinkEndChild(verticalsync);

	tinyxml2::XMLElement *multisample = document.NewElement("multisample");
	multisample->SetAttribute("samples", OPENGL_MULTISAMPLE);
	preferences->LinkEndChild(multisample);

	tinyxml2::XMLElement *motionblur = document.NewElement("motionblur");
	motionblur->SetAttribute("steps", MOTIONBLUR_STEPS);
	motionblur->SetAttribute("strength", xs_RoundToInt(MOTIONBLUR_TIME*6000));
	preferences->LinkEndChild(motionblur);

	tinyxml2::XMLElement *sound = document.NewElement("sound");
	sound->SetAttribute("channels", SOUND_CHANNELS);
	sound->SetAttribute("effectvolume", xs_RoundToInt(SOUND_VOLUME_EFFECT*100));
	sound->SetAttribute("musicvolume", xs_RoundToInt(SOUND_VOLUME_MUSIC*100));
	preferences->LinkEndChild(sound);

	document.SaveFile(config);
	return true;
}
