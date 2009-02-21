#include "StdAfx.h"

#include "oglconsole.h"
#include "World.h"
#include "Sound.h"
#include "GameState.h"


// console
extern OGLCONSOLE_Console console;

// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_CreateFont();
extern "C" void OGLCONSOLE_Resize(OGLCONSOLE_Console console);

// open window
extern bool OpenWindow(void);

// close window
extern void CloseWindow(void);

// init input
extern bool InitInput(const char *config);

// split level
extern bool SplitLevel(const char *config, const char *output);


// post-command function
typedef void (*ProcessCommandPostFunc)(void);

// process a string command
static int ProcessCommandString(std::string &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = aParam[0];
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue.c_str());
		return 0;
	}
}

// process a boolean command
static int ProcessCommandBool(bool &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = atoi(aParam[0]) != 0;
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue);
		return 0;
	}
}

// process an integer command
static int ProcessCommandInt(int &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = atoi(aParam[0]);
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue);
		return 0;
	}
}

// process a two-integer command
static int ProcessCommandInt2(int &aValue1, int &aValue2, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 2)
	{
		aValue1 = atoi(aParam[0]);
		aValue2 = atoi(aParam[1]);
		if (aAction)
			aAction();
		return 2;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue1, aValue2);
		return 0;
	}
}

// process a float command
static int ProcessCommandFloat(float &aValue, char *aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat)
{
	if (aCount >= 1)
	{
		aValue = float(atof(aParam[0]));
		if (aAction)
			aAction();
		return 1;
	}
	else
	{
		OGLCONSOLE_Output(console, aFormat, aValue);
		return 0;
	}
}

void UpdateWindowAction()
{
	if (runtime)
	{
		CloseWindow();
		OpenWindow();
	}
}
void InitInputAction()
{
	if (runtime)
		InitInput(INPUT_CONFIG.c_str());
}
void InitLevelAction()
{
	if (runtime)
	{
		if (curgamestate == STATE_PLAY)
		{
			setgamestate = STATE_RELOAD;
		}
	}
}
void InitRecordAction()
{
	record = 1;
}
void InitPlaybackAction()
{
	playback = 1;
}
void ClampMotionBlurAction()
{
	if (MOTIONBLUR_STEPS < 1)
		MOTIONBLUR_STEPS = 1;
}

// commands
int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount )
{
	switch (aCommand)
	{
	case 0x11e1fc01 /* "shell" */:
		setgamestate = STATE_SHELL;
		return 0;

	case 0xc2cbd863 /* "play" */:
		setgamestate = STATE_PLAY;
		return 0;

	case 0xed7cdd8c /* "reload" */:
		setgamestate = STATE_RELOAD;
		return 0;

	case 0x47878736 /* "quit" */:
		setgamestate = STATE_QUIT;
		return 0;

	case 0x1d215c8f /* "resolution" */:
		return ProcessCommandInt2(SCREEN_WIDTH, SCREEN_HEIGHT, aParam, aCount, UpdateWindowAction, "resolution: %dx%d\n"); 

	case 0xfe759eea /* "depth" */:
		return ProcessCommandInt(SCREEN_DEPTH, aParam, aCount, UpdateWindowAction, "depth: %dbpp");

	case 0x5032fb58 /* "fullscreen" */:
		return ProcessCommandBool(SCREEN_FULLSCREEN, aParam, aCount, UpdateWindowAction, "fullscreen: %d\n");

	case 0x06f8f066 /* "vsync" */:
		return ProcessCommandBool(OPENGL_SWAPCONTROL, aParam, aCount, UpdateWindowAction, "vsync: %d\n");

	case 0x35c8978f /* "antialias" */:
		return ProcessCommandBool(OPENGL_ANTIALIAS, aParam, aCount, UpdateWindowAction, "antialias: %d\n");

	case 0x47d0f228 /* "multisample" */:
		return ProcessCommandInt(OPENGL_MULTISAMPLE, aParam, aCount, UpdateWindowAction, "multisample: %d\n");

	case 0x1ae79789 /* "viewsize" */:
		return ProcessCommandFloat(VIEW_SIZE, aParam, aCount, NULL, "viewsize: %f\n");

	case 0xf9d86f7b /* "input" */:
		return ProcessCommandString(INPUT_CONFIG, aParam, aCount, InitInputAction, "input: %s\n");

	case 0x9b99e7dd /* "level" */:
		return ProcessCommandString(LEVEL_CONFIG, aParam, aCount, InitLevelAction, "level: %s\n");

	case 0x112a90d4 /* "import" */:
		if (aCount > 0)
		{
			// level configuration
			TiXmlDocument document(aParam[0]);
			if (!document.LoadFile())
				DebugPrint("error loading import file \"%s\": %s\n", aParam[0], document.ErrorDesc());

			// process child element
			if (const TiXmlElement *root = document.FirstChildElement())
				ProcessWorldItem(root);
			return 1;
		}
		return 0;

	case 0x87b82de3 /* "split" */:
		if (aCount > 0)
		{
			SplitLevel(LEVEL_CONFIG.c_str(), aParam[0]);
			return 1;
		}
		return 0;

	case 0x593058cc /* "record" */:
		return ProcessCommandString(RECORD_CONFIG, aParam, aCount, InitRecordAction, "record: %s\n");

	case 0xcf8a43ec /* "playback" */:
		return ProcessCommandString(RECORD_CONFIG, aParam, aCount, InitPlaybackAction, "playback: %s\n");

	case 0xd6974b06 /* "simrate" */:
		return ProcessCommandInt(SIMULATION_RATE, aParam, aCount, NULL, "simrate: %d\n");

	case 0x9f2f269e /* "timescale" */:
		return ProcessCommandFloat(TIME_SCALE, aParam, aCount, NULL, "timescale: %f\n");

	case 0xe065cb63 /* "fixedstep" */:
		return ProcessCommandBool(FIXED_STEP, aParam, aCount, NULL, "fixedstep: %d\n");

	case 0xf744f3b2 /* "motionblur" */:
		return ProcessCommandInt(MOTIONBLUR_STEPS, aParam, aCount, ClampMotionBlurAction, "motionblur: %d\n");

	case 0xfb585f73 /* "motionblurtime" */:
		return ProcessCommandFloat(MOTIONBLUR_TIME, aParam, aCount, NULL, "motionblurtime: %f\n");

	case 0x61e734dc /* "soundchannels" */:
		return ProcessCommandInt(SOUND_CHANNELS, aParam, aCount, NULL, "soundchannels: %d\n");

	case 0xff4838bb /* "soundvolumeeffect" */:
		return ProcessCommandFloat(SOUND_VOLUME_EFFECT, aParam, aCount, UpdateSoundVolume, "soundvolume: %f\n");
		
	case 0x689fc51d /* "soundvolumemusic" */:
		return ProcessCommandFloat(SOUND_VOLUME_MUSIC, aParam, aCount, UpdateSoundVolume, "soundvolume: %f\n");
		
	case 0x94c716fd /* "outputconsole" */:
		return ProcessCommandBool(DEBUGPRINT_OUTPUTCONSOLE, aParam, aCount, NULL, "outputconsole: %d\n");

	case 0x54822903 /* "outputdebug" */:
		return ProcessCommandBool(DEBUGPRINT_OUTPUTDEBUG, aParam, aCount, NULL, "outputdebug: %d\n");

	case 0x8940763c /* "outputstderr" */:
		return ProcessCommandBool(DEBUGPRINT_OUTPUTSTDERR, aParam, aCount, NULL, "outputstderr: %d\n");

	case 0xfbcc8f02 /* "profilescreen" */:
		return ProcessCommandBool(PROFILER_OUTPUTSCREEN, aParam, aCount, NULL, "profilescreen: %d\n");

	case 0x85e872f9 /* "profileprint" */:
		return ProcessCommandBool(PROFILER_OUTPUTPRINT, aParam, aCount, NULL, "profileprint: %d\n");

	case 0x24ce5450 /* "frameratescreen" */:
		return ProcessCommandBool(FRAMERATE_OUTPUTSCREEN, aParam, aCount, NULL, "frameratescreen: %d\n");

	case 0x55cfbc33 /* "framerateprint" */:
		return ProcessCommandBool(FRAMERATE_OUTPUTPRINT, aParam, aCount, NULL, "framerateprint: %d\n");

	case 0xa165ddb8 /* "database" */:
		if (aCount >= 1)
		{
			// get the database identifier
			unsigned int id;
			if (!TIXML_SSCANF(aParam[0], "0x%x", &id))
				id = Hash(aParam[0]);

			// get the dtabase
			Database::Untyped *db = Database::GetDatabases().Get(id);
			if (db)
			{
				// list database properties
				OGLCONSOLE_Output(console, "stride=%d shift=%d bits=%d limit=%d count=%d\n",
					db->GetStride(), db->GetShift(), db->GetBits(), db->GetLimit(), db->GetCount());
			}
			else
			{
				// not found
				OGLCONSOLE_Output(console, "database \"%s\" (0x%08x) not found\n", aParam[0], id);
			}
			return 1;
		}
		else
		{
			// list all database identifiers
			OGLCONSOLE_Output(console, "databases:\n");
			for (Database::Untyped::Iterator itor(&Database::GetDatabases()); itor.IsValid(); ++itor)
			{
				OGLCONSOLE_Output(console, "0x%08x\n", itor.GetKey());
			}
			return 0;
		}

	case 0xbdf0855a /* "find" */:
		if (aCount >= 1)
		{
			// get the database identifier
			unsigned int id;
			if (!TIXML_SSCANF(aParam[0], "0x%x", &id))
				id = Hash(aParam[0]);

			// get the database
			Database::Untyped *db = Database::GetDatabases().Get(id);
			if (db)
			{
				// if supplying a key...
				if (aCount >= 2)
				{
					// if the key is an identifier...
					unsigned int key = 0;
					if (TIXML_SSCANF(aParam[1], "0x%x", &key))
					{
						// look up the identifier
						if (const void *value = db->Find(key))
						{
							const std::string &name = Database::name.Get(key);
							OGLCONSOLE_Output(console, "name=\"%s\" key=0x%08x data=0x%p\n", name.c_str(), key, value);
						}
						else
						{
							OGLCONSOLE_Output(console, "no record found\n");
						}
					}
					else
					{
						// list all keys matching the string name
						OGLCONSOLE_Output(console, "records matching \"%s\":\n", aParam[1]);
						for (Database::Untyped::Iterator itor(db); itor.IsValid(); ++itor)
						{
							const std::string &name = Database::name.Get(itor.GetKey());
							if (_stricmp(name.c_str(), aParam[1]) == 0)
							{
								OGLCONSOLE_Output(console, "%d: name=\"%s\" key=0x%08x data=0x%p\n", itor.GetSlot(), name.c_str(), itor.GetKey(), itor.GetValue());
							}
						}
					}
					return 2;
				}
				else
				{
					// list the contents of the database
					for (Database::Untyped::Iterator itor(db); itor.IsValid(); ++itor)
					{
						const std::string &name = Database::name.Get(itor.GetKey());
						OGLCONSOLE_Output(console, "%d: name=\"%s\" key=0x%08x data=0x%p\n", itor.GetSlot(), name.c_str(), itor.GetKey(), itor.GetValue());
					}
				}
			}
			else
			{
				// not found
				OGLCONSOLE_Output(console, "database \"%s\" (0x%08x) not found\n", aParam[0], id);
			}
			return 1;
		}
		else
		{
			return 0;
		}

	case 0x1bf51169 /* "components" */:
		if (aCount >= 1)
		{
			// get the identifier
			unsigned int key;
			if (!TIXML_SSCANF(aParam[0], "0x%x", &key))
				key = Hash(aParam[0]);

			// for each database...
			for (Database::Typed<Database::Untyped *>::Iterator itor(&Database::GetDatabases()); itor.IsValid(); ++itor)
			{
				// if the database contains a record for the identifier...
				if (const void *value = itor.GetValue()->Find(key))
				{
					OGLCONSOLE_Output(console, "database 0x%08x data=0x%p\n", itor.GetKey(), value);
				}
			}
			return 1;
		}
		else
		{
			return 0;
		}

	case 0x0e0d9594 /* "sound" */:
		if (aCount >= 2)
		{
			PlaySoundCue(Hash(aParam[0]), Hash(aParam[1]));
			return 2;
		}
		else if (aCount == 1)
		{
			PlaySoundCue(Hash(aParam[0]));
			return 1;
		}
		else
		{
			return 0;
		}
		
	default:
#ifdef USE_VARIABLE
		// check variable system
		if (VarItem *item = Database::varitem.Get(aCommand))
		{
			if (item->mType == VarItem::COMMAND)
			{
				// trigger the command
				item->Notify();
				return 0;
			}
			else
			{
				if (aCount >= 1)
				{
					// set value
					item->SetString(aParam[0]);
					return 1;
				}
				else
				{
					// get value
					OGLCONSOLE_Output(console, "%s\n", item->GetString().c_str());
					return 0;
				}
			}
		}
		else
		{
			return 0;
		}
#else
		return 0;
#endif
	}
}
