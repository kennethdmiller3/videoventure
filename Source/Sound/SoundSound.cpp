#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"

static bool Configure(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id)
{
	// get sound template
	const char *name = element->Attribute("name");
	const SoundTemplate &sound = Database::soundtemplate.Get(Hash(name));
	if (sound.mLength)
	{
		// append sound data
		self.Reserve(sound.mLength);
		self.Append(static_cast<short *>(sound.mData), sound.mLength);
	}
	return true;
}

static SoundConfigure::Configure soundsound(0x0e0d9594 /* "sound" */, Configure);
