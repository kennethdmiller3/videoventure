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
		void AddConfigure(unsigned int aTagId, Entry aConfigure)
		{
			GetConfigureDB().Put(aTagId, aConfigure);
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
		const DeactivateEntry &GetDeactivate(unsigned int aDatabaseId)
		{
			return GetDeactivate().Get(aDatabaseId);
		}
	}
}