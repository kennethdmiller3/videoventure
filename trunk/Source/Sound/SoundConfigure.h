#pragma once

class SoundTemplate;

namespace SoundConfigure
{
	typedef fastdelegate::FastDelegate<bool (SoundTemplate &self, const TiXmlElement *element, unsigned int id)> Entry;

	void Add(unsigned int aTagId, Entry aEntry);
	void Remove(unsigned int aTagId);
	const Entry &Get(unsigned int aTagId);

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
