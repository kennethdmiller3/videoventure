#include "StdAfx.h"
#include "SoundConfigure.h"

namespace SoundConfigure
{
	Database::Typed<Entry> &GetDB()
	{
		static Database::Typed<Entry> configure;
		return configure;
	}
	void Add(unsigned int aTagId, Entry aConfigure)
	{
		GetDB().Put(aTagId, aConfigure);
	}
	void Remove(unsigned int aTagId)
	{
		GetDB().Delete(aTagId);
	}
	const Entry &Get(unsigned int aTagId)
	{
		return GetDB().Get(aTagId);
	}
}
