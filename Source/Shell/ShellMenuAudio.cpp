#include "StdAfx.h"
#include "ShellMenu.h"
#include "ShellMenuItem.h"
#include "VarItem.h"
#include "Preferences.h"
#include "Sound.h"


//
// AUDIO MENU

/*
channels				[-] <channels> [+]
effects volume			[-] <volume %> [+]
music volume			[-] <volume %> [+]
test					[-] <sound> [+]
*/

int shellmenuaudiosoundchannelsonenter;
char shellmenuaudiosoundchannelstext[8];
float shellmenuaudiosoundvolumeeffectonenter;
char shellmenuaudiosoundvolumeeffecttext[8];
float shellmenuaudiosoundvolumemusiconenter;
char shellmenuaudiosoundvolumemusictext[8];

void ShellMenuAudioEnter()
{
	shellmenuaudiosoundchannelsonenter = SOUND_CHANNELS;
	VarItem *varsoundchannels = VarItem::CreateInteger("shell.menu.audio.channels", SOUND_CHANNELS, 1);
	snprintf(shellmenuaudiosoundchannelstext, sizeof(shellmenuaudiosoundchannelstext), "%d", varsoundchannels->GetInteger());

	shellmenuaudiosoundvolumeeffectonenter = SOUND_VOLUME_EFFECT;
	VarItem *varsoundvolumeeffect = VarItem::CreateInteger("shell.menu.audio.volume.effect", int(SOUND_VOLUME_EFFECT * 10), 0, 20);
	snprintf(shellmenuaudiosoundvolumeeffecttext, sizeof(shellmenuaudiosoundvolumeeffecttext), "%d%%", varsoundvolumeeffect->GetInteger() * 10);

	shellmenuaudiosoundvolumemusiconenter = SOUND_VOLUME_MUSIC;
	VarItem *varsoundvolumemusic = VarItem::CreateInteger("shell.menu.audio.volume.music", int(SOUND_VOLUME_MUSIC * 10), 0, 20);
	snprintf(shellmenuaudiosoundvolumemusictext, sizeof(shellmenuaudiosoundvolumemusictext), "%d%%", varsoundvolumemusic->GetInteger() * 10);
}

void ShellMenuAudioExit()
{
}

void ShellMenuAudioPressAccept()
{
	SOUND_CHANNELS = VarItem::GetInteger("shell.menu.audio.channels");
	SOUND_VOLUME_EFFECT = VarItem::GetInteger("shell.menu.audio.volume.effect") / 10.0f;
	SOUND_VOLUME_MUSIC = VarItem::GetInteger("shell.menu.audio.volume.music") / 10.0f;

	WritePreferences("preferences.xml");

	UpdateSoundVolume();

	shellmenu.Pop();
}

void ShellMenuAudioPressCancel()
{
	SOUND_CHANNELS = shellmenuaudiosoundchannelsonenter;
	SOUND_VOLUME_EFFECT = shellmenuaudiosoundvolumeeffectonenter;
	SOUND_VOLUME_MUSIC = shellmenuaudiosoundvolumemusiconenter;

	UpdateSoundVolume();

	shellmenu.Pop();
}


void ShellMenuAudioPressSoundChannelsUp()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
		SOUND_CHANNELS = item->GetInteger();
	}
}

void ShellMenuAudioPressSoundChannelsDown()
{
	if (VarItem *item = Database::varitem.Get(0x2e3f9248 /* "shell.menu.audio.channels" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundchannelstext, "%d", item->GetInteger());
		SOUND_CHANNELS = item->GetInteger();
	}
}

void ShellMenuAudioPressSoundVolumeEffectUp()
{
	if (VarItem *item = Database::varitem.Get(0x686112dd /* "shell.menu.audio.volume.effect" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundvolumeeffecttext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_EFFECT = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
	}
}

void ShellMenuAudioPressSoundVolumeEffectDown()
{
	if (VarItem *item = Database::varitem.Get(0x686112dd /* "shell.menu.audio.volume.effect" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundvolumeeffecttext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_EFFECT = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
	}
}

void ShellMenuAudioPressSoundVolumeMusicUp()
{
	if (VarItem *item = Database::varitem.Get(0xea502ecf /* "shell.menu.audio.volume.music" */))
	{
		item->SetInteger(item->GetInteger() + 1);
		sprintf(shellmenuaudiosoundvolumemusictext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_MUSIC = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
	}
}

void ShellMenuAudioPressSoundVolumeMusicDown()
{
	if (VarItem *item = Database::varitem.Get(0xea502ecf /* "shell.menu.audio.volume.music" */))
	{
		item->SetInteger(item->GetInteger() - 1);
		sprintf(shellmenuaudiosoundvolumemusictext, "%d%%", item->GetInteger() * 10);
		SOUND_VOLUME_MUSIC = item->GetInteger() / 10.0f;
		UpdateSoundVolume();
	}
}

ShellMenuItem shellmenuaudioitems[] = 
{
	{
		Vector2( 40, 220 - 24 - 16 ),
		Vector2( 560, 12 ),
		optionbackcolor,
		"AUDIO",
		Vector2( 280, 6 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 32, 24 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_SELECTED,
		NULL,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 0 ),
		Vector2( 240, 24 ),
		NULL,
		"Mixer Channels",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundChannelsDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 0 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundchannelstext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 0 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundChannelsUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 1 ),
		Vector2( 240, 24 ),
		NULL,
		"Effects Volume",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 1 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeEffectDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 1 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundvolumeeffecttext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 1 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeEffectUp,
	},
	{
		Vector2( 320 - 20 - 240, 220 + 32 * 2 ),
		Vector2( 240, 24 ),
		NULL,
		"Music Volume",
		Vector2( 240 - 8, 12 ),
		Vector2( 1.0f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20, 220 + 32 * 2 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"-",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeMusicDown,
	},
	{
		Vector2( 320 + 20 + 30 + 10, 220 + 32 * 2 ),
		Vector2( 160, 24 ),
		NULL,
		shellmenuaudiosoundvolumemusictext,
		Vector2( 80, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		inertbordercolor,
		inertlabelcolor,
		BUTTON_NORMAL,
		NULL,
	},
	{
		Vector2( 320 + 20 + 30 + 10 + 160, 220 + 32 * 2 ),
		Vector2( 30, 24 ),
		optionbackcolor,
		"+",
		Vector2( 15, 12 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 16, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressSoundVolumeMusicUp,
	},
	{
		Vector2( 40, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"ACCEPT",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressAccept,
	},
	{
		Vector2( 600 - 240, 460 - 32 ),
		Vector2( 240, 32 ),
		optionbackcolor,
		"CANCEL",
		Vector2( 120, 16 ),
		Vector2( 0.5f, 0.5f ),
		Vector2( 24, 16 ),
		optionbordercolor,
		optionlabelcolor,
		BUTTON_NORMAL,
		ShellMenuAudioPressCancel,
	},
};
ShellMenuPage shellmenuaudiopage =
{
	shellmenuaudioitems, SDL_arraysize(shellmenuaudioitems), ShellMenuAudioEnter, ShellMenuAudioExit
};
