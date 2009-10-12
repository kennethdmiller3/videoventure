#pragma once

#include "DatabaseTyped.h"
#include "Signal.h"

namespace Database
{
	// component loaders
	namespace Loader
	{
		typedef fastdelegate::FastDelegate<void (unsigned int, const TiXmlElement *)> Entry;
		GAME_API void AddConfigure(unsigned int aTagId, Entry aEntry);
		GAME_API void RemoveConfigure(unsigned int aTagId, Entry aEntry);
		GAME_API const Entry &GetConfigure(unsigned int aTagId);
	}

	// component initializers
	namespace Initializer
	{
		typedef fastdelegate::FastDelegate<void (unsigned int)> Entry;
		GAME_API void AddActivate(unsigned int aDatabaseId, Entry aEntry);
		GAME_API void RemoveActivate(unsigned int aDatabaseId, Entry aEntry);
		GAME_API void AddPostActivate(unsigned int aDatabaseId, Entry aEntry);
		GAME_API void RemovePostActivate(unsigned int aDatabaseId, Entry aEntry);
		GAME_API void AddPreDeactivate(unsigned int aDatabaseId, Entry aEntry);
		GAME_API void RemovePreDeactivate(unsigned int aDatabaseId, Entry aEntry);
		GAME_API void AddDeactivate(unsigned int aDatabaseId, Entry aEntry);
		GAME_API void RemoveDeactivate(unsigned int aDatabaseId, Entry aEntry);
	}

	// get database of databases
	Typed<Untyped *> &GetDatabases();

	// name database
	extern GAME_API Typed<std::string> name;

	// parent identifier database
	extern GAME_API Typed<Key> parent;

	// owner identifier database
	extern GAME_API Typed<Key> owner;

	// creator identifier database
	extern GAME_API Typed<Key> creator;

	// deletion signal
	typedef Signal<void (Key)> DeleteSignal;
	extern GAME_API Typed<DeleteSignal> deleted;

	// instantiate a template
	void GAME_API Instantiate(unsigned int aInstanceId, unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity = Vector2(0, 0), float aOmega = 0, bool aActivate = true);
	unsigned int GAME_API Instantiate(unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity = Vector2(0, 0), float aOmega = 0, bool aActivate = true);

	// inherit from a template
	void GAME_API Inherit(unsigned int aInstanceId, unsigned int aTemplateId);

	// change an instance's type
	void GAME_API Switch(unsigned int aInstanceId, unsigned int aTemplateId);

	// activate an identifier
	void GAME_API Activate(unsigned int aId);
	
	// deactivate an identifier
	void GAME_API Deactivate(unsigned int aId);

	// delete an identifier
	void GAME_API Delete(unsigned int aid);

	// update the database system
	void Update(void);

	// clean up all databases
	void Cleanup(void);
}
