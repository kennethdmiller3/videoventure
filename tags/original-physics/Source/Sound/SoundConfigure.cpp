#include "StdAfx.h"
#include "SoundConfigure.h"

namespace SoundConfigure
{
	Database::Typed<Entry> &Configure::GetDB()
	{
		static Database::Typed<Entry> onactivate;
		return onactivate;
	}
	Configure::Configure(unsigned int aTagId, Entry aEntry)
		: mTagId(aTagId)
	{
		Database::Typed<Entry> &db = GetDB();
		Entry &entry = db.Open(mTagId);
		mPrev = entry;
		entry = aEntry;
		db.Close(mTagId);
	}
	Configure::~Configure()
	{
		Database::Typed<Entry> &db = GetDB();
		if (mPrev)
			db.Put(mTagId, mPrev);
		else
			db.Delete(mTagId);
	}
	const Entry &Configure::Get(unsigned int aTagId)
	{
		return GetDB().Get(aTagId);
	}
}
