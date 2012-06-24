#pragma once

class SoundTemplate;

namespace SoundConfigure
{
	typedef bool (*Entry)(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id);

	class GAME_API Configure
	{
	private:
		unsigned int mTagId;	// tag hash id for the configure
		Entry mPrev;			// entry that this replaced

	public:
		static Database::Typed<Entry> &GetDB();
		Configure(unsigned int aTagId, Entry aEntry);
		~Configure();
		static const Entry &Get(unsigned int aTagId);
	};
}
