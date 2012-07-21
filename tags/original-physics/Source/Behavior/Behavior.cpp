#include "StdAfx.h"
#include "Behavior.h"

namespace BehaviorDatabase
{
	//
	// LOADER SYSTEM
	//
	namespace Loader
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

	//
	// INITIALIZER SYSTEM
	//
	namespace Initializer
	{
		Database::Typed<Activate::Entry> &Activate::GetDB()
		{
			static Database::Typed<Entry> onactivate;
			return onactivate;
		}
		Activate::Activate(unsigned int aDatabaseId, Entry aEntry)
			: mDatabaseId(aDatabaseId)
		{
			Database::Typed<Entry> &db = GetDB();
			Entry &entry = db.Open(mDatabaseId);
			mPrev = entry;
			entry = aEntry;
			db.Close(mDatabaseId);
		}
		Activate::~Activate()
		{
			Database::Typed<Entry> &db = GetDB();
			if (mPrev)
				db.Put(mDatabaseId, mPrev);
			else
				db.Delete(mDatabaseId);
		}
		const Activate::Entry &Activate::Get(unsigned int aDatabaseId)
		{
			return GetDB().Get(aDatabaseId);
		}

		Database::Typed<Deactivate::Entry> &Deactivate::GetDB()
		{
			static Database::Typed<Entry> ondeactivate;
			return ondeactivate;
		}
		Deactivate::Deactivate(unsigned int aDatabaseId, Entry aEntry)
			: mDatabaseId(aDatabaseId)
		{
			Database::Typed<Entry> &db = GetDB();
			Entry &entry = db.Open(mDatabaseId);
			mPrev = entry;
			entry = aEntry;
			db.Close(mDatabaseId);
		}
		Deactivate::~Deactivate()
		{
			Database::Typed<Entry> &db = GetDB();
			if (mPrev)
				db.Put(mDatabaseId, mPrev);
			else
				db.Delete(mDatabaseId);
		}
		const Deactivate::Entry &Deactivate::Get(unsigned int aDatabaseId)
		{
			return GetDB().Get(aDatabaseId);
		}
	}
}