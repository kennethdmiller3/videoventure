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
		Typed<Entry> &GetConfigureDB()
		{
			static Typed<Entry> configure;
			return configure;
		}
		void AddConfigure(unsigned int aTagId, Entry aEntry)
		{
			GetConfigureDB().Put(aTagId, aEntry);
		}
		void RemoveConfigure(unsigned int aTagId, Entry aEntry)
		{
			if (GetConfigureDB().Get(aTagId) == aEntry);
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
		Typed<Entry> &GetActivate()
		{
			static Typed<Entry> onactivate;
			return onactivate;
		}
		void AddActivate(unsigned int aDatabaseId, Entry aEntry)
		{
			GetActivate().Put(aDatabaseId, aEntry);
		}
		void RemoveActivate(unsigned int aDatabaseId, Entry aEntry)
		{
			if (GetActivate().Get(aDatabaseId) == aEntry)
				GetActivate().Delete(aDatabaseId);
		}
		Typed<Entry> &GetPostActivate()
		{
			static Typed<Entry> onpostactivate;
			return onpostactivate;
		}
		void AddPostActivate(unsigned int aDatabaseId, Entry aEntry)
		{
			GetPostActivate().Put(aDatabaseId, aEntry);
		}
		void RemovePostActivate(unsigned int aDatabaseId, Entry aEntry)
		{
			if (GetPostActivate().Get(aDatabaseId) == aEntry)
				GetPostActivate().Delete(aDatabaseId);
		}
		Typed<Entry> &GetPreDeactivate()
		{
			static Typed<Entry> onpredeactivate;
			return onpredeactivate;
		}
		void AddPreDeactivate(unsigned int aDatabaseId, Entry aEntry)
		{
			GetPreDeactivate().Put(aDatabaseId, aEntry);
		}
		void RemovePreDeactivate(unsigned int aDatabaseId, Entry aEntry)
		{
			if (GetPreDeactivate().Get(aDatabaseId) == aEntry)
				GetPreDeactivate().Delete(aDatabaseId);
		}
		Typed<Entry> &GetDeactivate()
		{
			static Typed<Entry> ondeactivate;
			return ondeactivate;
		}
		void AddDeactivate(unsigned int aDatabaseId, Entry aEntry)
		{
			GetDeactivate().Put(aDatabaseId, aEntry);
		}
		void RemoveDeactivate(unsigned int aDatabaseId, Entry aEntry)
		{
			if (GetDeactivate().Get(aDatabaseId) == aEntry)
				GetDeactivate().Delete(aDatabaseId);
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

	// activate an identifier
	void Activate(unsigned int aId)
	{
		// queue activation
		activatequeue.push_back(aId);

		// defer activation if busy
		if (activatequeue.size() > 1)
			return;

		// process queued activations
		while (!activatequeue.empty())
		{
			// get the first entry
			unsigned int aId = activatequeue.front();

			// for each activation initializer...
			for (Typed<Initializer::Entry>::Iterator itor(&Initializer::GetActivate()); itor.IsValid(); ++itor)
			{
				// if the corresponding database has a record...
				if (GetDatabases().Get(itor.GetKey())->Find(aId))
				{
					// call the initializer
					itor.GetValue()(aId);
				}
			}

			// if entity has no physics...
			if (!Database::collidable.Get(aId))
			{
				// get the entity
				Entity *entity = Database::entity.Get(aId);

				// step the entity (HACK)
				// TO DO: come up with a workaround for this
				entity->Step();
			}

			// for each post-activation initializer...
			for (Typed<Initializer::Entry>::Iterator itor(&Initializer::GetPostActivate()); itor.IsValid(); ++itor)
			{
				// if the corresponding database has a record...
				if (GetDatabases().Get(itor.GetKey())->Find(aId))
				{
					// call the initializer
					itor.GetValue()(aId);
				}
			}

			// remove from the queue
			activatequeue.pop_front();
		}
	}

	// deactivate an identifier
	void Deactivate(unsigned int aId)
	{
		// queue deactivation
		deactivatequeue.push_back(aId);

		// defer deactivation if busy
		if (deactivatequeue.size() > 1)
			return;

		// process queued deactivations
		while (!deactivatequeue.empty())
		{
			// get the first entry
			unsigned int aId = deactivatequeue.front();

			// for each pre-deactivation initializer...
			for (Typed<Initializer::Entry>::Iterator itor(&Initializer::GetPreDeactivate()); itor.IsValid(); ++itor)
			{
				// if the corresponding database has a record...
				if (GetDatabases().Get(itor.GetKey())->Find(aId))
				{
					// call the initializer
					itor.GetValue()(aId);
				}
			}

			// for each deactivation initializer...
			for (Typed<Initializer::Entry>::Iterator itor(&Initializer::GetDeactivate()); itor.IsValid(); ++itor)
			{
				// if the corresponding database has a record...
				if (GetDatabases().Get(itor.GetKey())->Find(aId))
				{
					// call the initializer
					itor.GetValue()(aId);
				}
			}

			// remove from the queue
			deactivatequeue.pop_front();
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

	// destroy the identifier
	void Destroy(unsigned int aId)
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

	// update the database system
	void Update(void)
	{
		while (!deletequeue.empty())
		{
			unsigned int aId = deletequeue.front();
			deletequeue.pop_front();
			Destroy(aId);
		}
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
