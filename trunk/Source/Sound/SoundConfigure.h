#pragma once

class SoundTemplate;

namespace SoundConfigure
{
	typedef fastdelegate::FastDelegate<bool (SoundTemplate &self, const TiXmlElement *element, unsigned int id)> Entry;
	void Add(unsigned int aTagId, Entry aEntry);
	const Entry &Get(unsigned int aTagId);

	struct Auto
	{
		Auto(unsigned int aTagId, Entry aEntry)
		{
			Add(aTagId, aEntry);
		}
	};
}
