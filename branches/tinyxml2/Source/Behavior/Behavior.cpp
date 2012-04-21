#include "StdAfx.h"
#include "Behavior.h"

namespace BehaviorDatabase
{
	//
	// LOADER SYSTEM
	//
	namespace Loader
	{
		Database::Typed<Entry> &GetConfigureDB()
		{
			static Database::Typed<Entry> configure;
			return configure;
		}
		void AddConfigure(unsigned int aTagId, Entry aEntry)
		{
			GetConfigureDB().Put(aTagId, aEntry);
		}
		void RemoveConfigure(unsigned int aTagId, Entry aEntry)
		{
			if (GetConfigureDB().Get(aTagId) == aEntry)
				GetConfigureDB().Delete(aTagId);
		}
		const Entry &GetConfigure(unsigned int aTagId)
		{
			return GetConfigureDB().Get(aTagId);
		}
	}

	//
	// INITIALIZER SYSTEM
	//
	namespace Initializer
	{
		Database::Typed<ActivateEntry> &GetActivate()
		{
			static Database::Typed<ActivateEntry> onactivate;
			return onactivate;
		}
		void AddActivate(unsigned int aDatabaseId, ActivateEntry aEntry)
		{
			GetActivate().Put(aDatabaseId, aEntry);
		}
		void RemoveActivate(unsigned int aDatabaseId, ActivateEntry aEntry)
		{
			if (GetActivate().Get(aDatabaseId) == aEntry)
				GetActivate().Delete(aDatabaseId);
		}
		const ActivateEntry &GetActivate(unsigned int aDatabaseId)
		{
			return GetActivate().Get(aDatabaseId);
		}

		Database::Typed<DeactivateEntry> &GetDeactivate()
		{
			static Database::Typed<DeactivateEntry> ondeactivate;
			return ondeactivate;
		}
		void AddDeactivate(unsigned int aDatabaseId, DeactivateEntry aEntry)
		{
			GetDeactivate().Put(aDatabaseId, aEntry);
		}
		void RemoveDeactivate(unsigned int aDatabaseId, DeactivateEntry aEntry)
		{
			if (GetDeactivate().Get(aDatabaseId) == aEntry)
				GetDeactivate().Delete(aDatabaseId);
		}
		const DeactivateEntry &GetDeactivate(unsigned int aDatabaseId)
		{
			return GetDeactivate().Get(aDatabaseId);
		}
	}
}