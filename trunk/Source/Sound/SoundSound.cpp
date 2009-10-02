#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"

static bool Configure(SoundTemplate &self, const TiXmlElement *element, unsigned int id)
{
	// get sound template
	const char *name = element->Attribute("name");
	const SoundTemplate &sound = Database::soundtemplate.Get(Hash(name));
	if (sound.mLength)
	{
		// append sound data
		self.mSize = (self.mLength + sound.mLength) * sizeof(short);
		self.mData = realloc(self.mData, self.mSize);
		memcpy(static_cast<short *>(self.mData) + self.mLength, sound.mData, sound.mLength * sizeof(short));
	}
	return true;
}

static SoundConfigure::Auto soundsound(0x0e0d9594 /* "sound" */, Configure);
