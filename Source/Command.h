#pragma once

namespace Command
{
	typedef fastdelegate::FastDelegate<int (const char * const aParam[], int aCount)> Entry;
	void Add(unsigned int aTagId, Entry aEntry);
	void Remove(unsigned int aTagId);
	const Entry &Get(unsigned aTagId);
	struct Auto
	{
		unsigned int mTagId;

		Auto(unsigned int aTagId, Entry aEntry)
			: mTagId(aTagId)
		{
			Add(mTagId, aEntry);
		}
		~Auto()
		{
			Remove(mTagId);
		}
	};
}

// post-command function
typedef void (*ProcessCommandPostFunc)(void);

// process a string command
int ProcessCommandString(std::string &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandBool(bool &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandInt(int &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandInt2(int &aValue1, int &aValue2, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandFloat(float &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);

extern int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );
