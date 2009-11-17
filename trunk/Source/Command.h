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

extern int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );
