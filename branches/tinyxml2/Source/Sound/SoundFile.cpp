#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "SoundUtilities.h"

#if defined(USE_BASS)

#include "bass.h"
extern const char * BASS_ErrorGetString();

#elif defined(USE_SDL_MIXER)

#include "SDL_Mixer.h"

#else

#include "SoundMixer.h"

#endif

static bool Configure(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id)
{
	const char *name = element->Attribute("name");
	if (name == NULL)
		return false;

#if defined(USE_BASS)

	// load sound file
	HSAMPLE handle = BASS_SampleLoad(false, name, 0, 0, 1, BASS_SAMPLE_MONO);
	if (!handle)
	{
		DebugPrint("error loading sound file \"%s\": %s\n", name, BASS_ErrorGetString());
		return false;
	}

	// get sample info
	BASS_SAMPLE info;
	BASS_SampleGetInfo(handle, &info);

	// allocate space for data
	self.Reserve(info.length / info.chans);

	// set frequency
	self.mFrequency = info.freq;

	// if converting format...
	if ((info.chans > 1) || (info.flags & BASS_SAMPLE_8BITS))
	{
		// get sample data
		void *buf = _alloca(info.length);
		BASS_SampleGetData(handle, buf);

		// if converting from 8-bit...
		int accum = 0;
		if (info.flags & BASS_SAMPLE_8BITS)
		{
			for (unsigned int in = 0, samp = 0; in < info.length; ++in)
			{
				accum += static_cast<unsigned char *>(buf)[in];
				if (++samp >= info.chans)
				{
					self.Append(short(accum * 257 / samp - 32768));
					accum = 0;
					samp = 0;
				}
			}
		}
		else
		{
			for (unsigned int in = 0, samp = 0; in < info.length / sizeof(short); ++in)
			{
				accum += static_cast<short *>(buf)[in];
				if (++samp >= info.chans)
				{
					self.Append(short(accum / samp));
					accum = 0;
					samp = 0;
				}
			}
		}
	}
	else
	{
		// copy sound data
		BASS_SampleGetData(handle, static_cast<short *>(self.mData) + self.mLength);
	}

	// free loaded data
	BASS_SampleFree(handle);

#elif defined(USE_SDL_MIXER)

	// load sound file
	Mix_Chunk *loadchunk = Mix_LoadWAV(name);
	if (!loadchunk)
	{
		DebugPrint("error loading sound file \"%s\": %s\n", name, Mix_GetError());
		return false;
	}

	// copy sound data
	self.mSize = self.mLength * sizeof(short) + loadchunk->alen;
	self.mData = realloc(self.mData, self.mSize);
	memcpy(static_cast<short *>(self.mData) + self.mLength, loadchunk->abuf, loadchunk->alen);
	self.mLength = self.mSize / sizeof(short);

	// free loaded data
	Mix_FreeChunk(loadchunk);

#elif defined(USE_SDL)

	// load wave file data
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	if ( !SDL_LoadWAV(name, &wave, &data, &dlen) )
	{
		DebugPrint("error loading sound file \"%s\": %s\n", name, SDL_GetError());
		return false;
	}

	// build audio conversion
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq,
							AUDIO_S16,   1,             AUDIO_FREQUENCY);

	// append sound data
	self.mSize = self.mLength * sizeof(short) + dlen * cvt.len_mult;
	self.mData = realloc(self.mData, self.mSize);
	memcpy(static_cast<short *>(self.mData) + self.mLength, data, dlen);
	cvt.buf = reinterpret_cast<unsigned char *>(static_cast<short *>(mData) + mLength);
	cvt.len = dlen;

	// convert to final format
	SDL_ConvertAudio(&cvt);
	self.mLength += cvt.len_cvt / sizeof(short);
	self.mSize = self.mLength * sizeof(short);
	self.mData = realloc(self.mData, self.mSize);

	// release wave file data
	SDL_FreeWAV(data);

#endif

	return true;
}

static SoundConfigure::Auto soundfileloader(0xaaea5743 /* "file" */, Configure);
