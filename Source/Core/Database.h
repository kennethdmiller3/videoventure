#pragma once

#include "DatabaseTyped.h"
#include "Signal.h"

namespace Database
{
	// component loaders
	namespace Loader
	{
		typedef void (*Entry)(unsigned int, const tinyxml2::XMLElement *);

		class GAME_API Configure
		{
		private:
			unsigned int mTagId;	// tag hash id for the configure
			Entry mPrev;			// entry that this replaced

		public:
			static Typed<Entry> &GetDB();
			Configure(unsigned int aTagId, Entry aEntry);
			~Configure();
			static const Entry &Get(unsigned int aTagId);
		};
	}

	// component initializers
	namespace Initializer
	{
		typedef void (*Entry)(unsigned int);

		class GAME_API Activate
		{
		private:
			unsigned int mDatabaseId;
			Entry mPrev;

		public:
			static Typed<Entry> &GetDB();
			Activate(unsigned int aDatabaseId, Entry aEntry);
			~Activate();
		};

		class GAME_API PostActivate
		{
		private:
			unsigned int mDatabaseId;
			Entry mPrev;

		public:
			static Typed<Entry> &GetDB();
			PostActivate(unsigned int aDatabaseId, Entry aEntry);
			~PostActivate();
		};

		class GAME_API PreDeactivate
		{
		private:
			unsigned int mDatabaseId;
			Entry mPrev;

		public:
			static Typed<Entry> &GetDB();
			PreDeactivate(unsigned int aDatabaseId, Entry aEntry);
			~PreDeactivate();
		};

		class GAME_API Deactivate
		{
		private:
			unsigned int mDatabaseId;
			Entry mPrev;

		public:
			static Typed<Entry> &GetDB();
			Deactivate(unsigned int aDatabaseId, Entry aEntry);
			~Deactivate();
		};
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
