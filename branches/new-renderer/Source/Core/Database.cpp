#include "stdafx.h"
#include "Database.h"
#include "Entity.h"
#include "Collidable.h"

namespace Database
{
	// get database of databases
	// (wrapped in a function to handle global initialization order)
	Typed<Untyped *> &GetDatabases()
	{
		static Typed<Untyped *> databases;
		return databases;
	}

	// name database
	Typed<std::string> name(0x8d39bde6 /* "name" */);

	// parent identifier database
	Typed<Key> parent(0xeacdfcfd /* "parent" */);

	// owner identifier database
	Typed<Key> owner(0xf5674cd4 /* "owner" */);

	// creator identifier database
	Typed<Key> creator(0x27d56017 /* "creator" */);

	// deleted signal
	Typed<DeleteSignal> deleted(0x84d1546a /* "deleted" */);

	//
	// LOADER SYSTEM
	//
	namespace Loader
	{
		Typed<Entry> &Configure::GetDB()
		{
			static Typed<Entry> onactivate;
			return onactivate;
		}
		Configure::Configure(unsigned int aTagId, Entry aEntry)
			: mTagId(aTagId)
		{
			Typed<Entry> &db = GetDB();
			Entry &entry = db.Open(mTagId);
			mPrev = entry;
			entry = aEntry;
			db.Close(mTagId);
		}
		Configure::~Configure()
		{
			Typed<Entry> &db = GetDB();
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
		Typed<Entry> &Activate::GetDB()
		{
			static Typed<Entry> onactivate;
			return onactivate;
		}
		Activate::Activate(unsigned int aDatabaseId, Entry aEntry)
			: mDatabaseId(aDatabaseId)
		{
			Typed<Entry> &db = GetDB();
			Entry &entry = db.Open(mDatabaseId);
			mPrev = entry;
			entry = aEntry;
			db.Close(mDatabaseId);
		}
		Activate::~Activate()
		{
			Typed<Entry> &db = GetDB();
			if (mPrev)
				db.Put(mDatabaseId, mPrev);
			else
				db.Delete(mDatabaseId);
		}

		Typed<Entry> &PostActivate::GetDB()
		{
			static Typed<Entry> onpostactivate;
			return onpostactivate;
		}
		PostActivate::PostActivate(unsigned int aDatabaseId, Entry aEntry)
			: mDatabaseId(aDatabaseId)
		{
			Typed<Entry> &db = GetDB();
			Entry &entry = db.Open(mDatabaseId);
			mPrev = entry;
			entry = aEntry;
			db.Close(mDatabaseId);
		}
		PostActivate::~PostActivate()
		{
			Typed<Entry> &db = GetDB();
			if (mPrev)
				db.Put(mDatabaseId, mPrev);
			else
				db.Delete(mDatabaseId);
		}

		Typed<Entry> &PreDeactivate::GetDB()
		{
			static Typed<Entry> onpredeactivate;
			return onpredeactivate;
		}
		PreDeactivate::PreDeactivate(unsigned int aDatabaseId, Entry aEntry)
			: mDatabaseId(aDatabaseId)
		{
			Typed<Entry> &db = GetDB();
			Entry &entry = db.Open(mDatabaseId);
			mPrev = entry;
			entry = aEntry;
			db.Close(mDatabaseId);
		}
		PreDeactivate::~PreDeactivate()
		{
			Typed<Entry> &db = GetDB();
			if (mPrev)
				db.Put(mDatabaseId, mPrev);
			else
				db.Delete(mDatabaseId);
		}

		Typed<Entry> &Deactivate::GetDB()
		{
			static Typed<Entry> ondeactivate;
			return ondeactivate;
		}
		Deactivate::Deactivate(unsigned int aDatabaseId, Entry aEntry)
			: mDatabaseId(aDatabaseId)
		{
			Typed<Entry> &db = GetDB();
			Entry &entry = db.Open(mDatabaseId);
			mPrev = entry;
			entry = aEntry;
			db.Close(mDatabaseId);
		}
		Deactivate::~Deactivate()
		{
			Typed<Entry> &db = GetDB();
			if (mPrev)
				db.Put(mDatabaseId, mPrev);
			else
				db.Delete(mDatabaseId);
		}
	}

	//
	// COMPONENT SYSTEM
	//

	// activation queue
	std::deque<unsigned int> activatequeue;

	// deactivation queue
	std::deque<unsigned int> deactivatequeue;

	// deletion queue
	std::deque<unsigned int> deletequeue;

	// instantiate a template
	void Instantiate(unsigned int aInstanceId, unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity, float aOmega, bool aActivate)
	{
		// set parent
		parent.Put(aInstanceId, aTemplateId);

		// set owner
		owner.Put(aInstanceId, aOwnerId);

		// set creator
		creator.Put(aInstanceId, aCreatorId);

		// create a new entity
		Entity *entity = new Entity(aInstanceId);
		entity->SetTransform(aAngle, aPosition);
		entity->SetVelocity(aVelocity);
		entity->SetOmega(aOmega);
		entity->Step();
		Database::entity.Put(aInstanceId, entity);

		if (aActivate)
		{
			// activate the instance identifier
			Activate(aInstanceId);
		}
	}

	// instantiate a template (automatically-generated identifier)
	unsigned int Instantiate(unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity, float aOmega, bool aActivate)
	{
		// generate an instance identifier
		const unsigned int aInstanceTag = Entity::TakeId();
		const unsigned int aInstanceId = Hash(&aInstanceTag, sizeof(aInstanceTag), aTemplateId);

		// instantiate the template
		Instantiate(aInstanceId, aTemplateId, aOwnerId, aCreatorId, aAngle, aPosition, aVelocity, aOmega, aActivate);

		// return the instance identifier
		return aInstanceId;
	}

	// inherit from a template
	void Inherit(unsigned int aInstanceId, unsigned int aTemplateId)
	{
		// for each registered database...
		for (Typed<Untyped *>::Iterator itor(&GetDatabases()); itor.IsValid(); ++itor)
		{
			// get the database
			Untyped *database = itor.GetValue();

			// if the database has a record for the template...
			if (const void *data = database->Find(aTemplateId))
			{
				// make sure there isn't already a record for the instance
				//assert(!database->Find(aInstanceId));

				// duplicate into the instance
				database->Put(aInstanceId, data);
			}
		}
	}

	// change an instance's type
	void Switch(unsigned int aInstanceId, unsigned int aTemplateId)
	{
		Deactivate(aInstanceId);
		parent.Put(aInstanceId, aTemplateId);
		Activate(aInstanceId);
	}

	// activate immediately
	void ActivateImmediate(unsigned int aId)
	{
		// for each activation initializer...
		for (Typed<Initializer::Entry>::Iterator itor(&Initializer::Activate::GetDB()); itor.IsValid(); ++itor)
		{
			// get the initializer
			if (Initializer::Entry initializer = itor.GetValue())
			{
				// get the corresponding database
				Untyped *database = GetDatabases().Get(itor.GetKey());

				// if the database exists and has a record...
				if (database && database->Find(aId))
				{
					// call the initializer
					initializer(aId);
				}
			}
		}

		// if entity has no physics...
		if (!Database::collidablebody.Get(aId))
		{
			// get the entity
			Entity *entity = Database::entity.Get(aId);

			// step the entity (HACK)
			// TO DO: come up with a workaround for this
			entity->Step();
		}

		// for each post-activation initializer...
		for (Typed<Initializer::Entry>::Iterator itor(&Initializer::PostActivate::GetDB()); itor.IsValid(); ++itor)
		{
			// get the initializer
			if (Initializer::Entry initializer = itor.GetValue())
			{
				// get the corresponding database
				Untyped *database = GetDatabases().Get(itor.GetKey());

				// if the database exists and has a record...
				if (database && database->Find(aId))
				{
					// call the initializer
					initializer(aId);
				}
			}
		}
	}

	// process queued activations
	void ActivateQueued()
	{
		// while the queue is not empty...
		while (!activatequeue.empty())
		{
			// get the first entry
			unsigned int aId = activatequeue.front();

			// activate the entry
			ActivateImmediate(aId);

			// remove from the queue
			activatequeue.pop_front();
		}
	}

	// activate an identifier
	void Activate(unsigned int aId)
	{
		// queue activation
		activatequeue.push_back(aId);

		// defer activation if busy
		if (!runtime || activatequeue.size() > 1)
			return;

		// process queued activations
		ActivateQueued();
	}

	// deactivate immediately
	void DeactivateImmediate(unsigned int aId)
	{
		// for each pre-deactivation initializer...
		for (Typed<Initializer::Entry>::Iterator itor(&Initializer::PreDeactivate::GetDB()); itor.IsValid(); ++itor)
		{
			// get the initializer
			if (Initializer::Entry initializer = itor.GetValue())
			{
				// get the corresponding database
				Untyped *database = GetDatabases().Get(itor.GetKey());

				// if the database exists and has a record...
				if (database && database->Find(aId))
				{
					// call the initializer
					initializer(aId);
				}
			}
		}

		// for each deactivation initializer...
		for (Typed<Initializer::Entry>::Iterator itor(&Initializer::Deactivate::GetDB()); itor.IsValid(); ++itor)
		{
			// get the initializer
			if (Initializer::Entry initializer = itor.GetValue())
			{
				// get the corresponding database
				Untyped *database = GetDatabases().Get(itor.GetKey());

				// if the database exists and has a record...
				if (database && database->Find(aId))
				{
					// call the initializer
					initializer(aId);
				}
			}
		}
	}

	// process queued deactivations
	void DeactivateQueued()
	{
		// while the queue is not empty...
		while (!deactivatequeue.empty())
		{
			// get the first entry
			unsigned int aId = deactivatequeue.front();

			// deactivate the entry
			DeactivateImmediate(aId);

			// remove from the queue
			deactivatequeue.pop_front();
		}
	}

	// deactivate an identifier
	void Deactivate(unsigned int aId)
	{
		// queue deactivation
		deactivatequeue.push_back(aId);

		// defer deactivation if busy
		if (!runtime || deactivatequeue.size() > 1)
			return;

		// process queued deactivations
		DeactivateQueued();
	}

	// delete immediately
	void DeleteImmediate(unsigned int aId)
	{
		// deactivate
		Deactivate(aId);

		// delete the entity
		if (Entity *entity = Database::entity.Get(aId))
		{
			delete entity;
		}

		// for each registered database...
		for (Typed<Untyped *>::Iterator itor(&GetDatabases()); itor.IsValid(); ++itor)
		{
			// get the database
			Untyped *database = itor.GetValue();

			// delete the record
			database->Delete(aId);
		}
	}

	// process queued deletions
	void DeleteQueued()
	{
		// while the queue is not empty...
		while (!deletequeue.empty())
		{
			// get the first entry
			unsigned int aId = deletequeue.front();

			// delete the entry
			DeleteImmediate(aId);

			// remove from the queue
			deletequeue.pop_front();
		}
	}

	// delete an identifier
	void Delete(unsigned int aId)
	{
		// defer deletion
		deletequeue.push_back(aId);

		// send out a signal
		deleted.Get(aId)(aId);
	}

	// pause
	void Pause(void);

	// resume
	void Resume(void);

	// update the database system
	void Update(void)
	{
		ActivateQueued();
		DeactivateQueued();
		DeleteQueued();
	}

	// clean up all databases
	void Cleanup(void)
	{
		// clear queues
		activatequeue.clear();
		deactivatequeue.clear();
		deletequeue.clear();

		// deactivate all instances
		for (Typed<Entity *>::Iterator itor(&entity); itor.IsValid(); ++itor)
		{
			Deactivate(itor.GetKey());
			delete itor.GetValue();
		}

		// clear all registered databases
		for (Typed<Untyped *>::Iterator itor(&GetDatabases()); itor.IsValid(); ++itor)
			itor.GetValue()->Clear();
	}
}
