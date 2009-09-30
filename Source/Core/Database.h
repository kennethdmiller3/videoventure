#pragma once

#include "DatabaseTyped.h"
#include "Signal.h"

namespace Database
{
	// component loaders
	namespace Loader
	{
		typedef fastdelegate::FastDelegate<void (unsigned int, const TiXmlElement *)> Entry;
		void AddConfigure(unsigned int aTagId, Entry aEntry);
		const Entry &GetConfigure(unsigned int aTagId);
	}

	// component initializers
	namespace Initializer
	{
		typedef fastdelegate::FastDelegate<void (unsigned int)> Entry;
		void AddActivate(unsigned int aDatabaseId, Entry aEntry);
		void AddPostActivate(unsigned int aDatabaseId, Entry aEntry);
		void AddPreDeactivate(unsigned int aDatabaseId, Entry aEntry);
		void AddDeactivate(unsigned int aDatabaseId, Entry aEntry);
	}

	// get database of databases
	Typed<Untyped *> &GetDatabases();

	// name database
	extern Typed<std::string> name;

	// parent identifier database
	extern Typed<Key> parent;

	// owner identifier database
	extern Typed<Key> owner;

	// creator identifier database
	extern Typed<Key> creator;

	// deletion signal
	typedef Signal<void (Key)> DeleteSignal;
	extern Typed<DeleteSignal> deleted;

	// instantiate a template
	void Instantiate(unsigned int aInstanceId, unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity = Vector2(0, 0), float aOmega = 0, bool aActivate = true);
	unsigned int Instantiate(unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity = Vector2(0, 0), float aOmega = 0, bool aActivate = true);

	// inherit from a template
	void Inherit(unsigned int aInstanceId, unsigned int aTemplateId);

	// change an instance's type
	void Switch(unsigned int aInstanceId, unsigned int aTemplateId);

	// activate an identifier
	void Activate(unsigned int aId);
	
	// deactivate an identifier
	void Deactivate(unsigned int aId);

	// delete an identifier
	void Delete(unsigned int aid);

	// update the database system
	void Update(void);

	// clean up all databases
	void Cleanup(void);
}
