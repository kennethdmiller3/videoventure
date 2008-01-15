#include "stdafx.h"
#include "Database.h"
#include "Entity.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Damagable.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Spawner.h"
#include "Player.h"
#include "Gunner.h"
#include "Aimer.h"
#include <new>

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

	// team identifier database
	Typed<unsigned int> team(0xa2fd7d0c /* "team" */);


	//
	// UNTYPED DATABASE
	//

	// constructor
	Untyped::Untyped(unsigned int aId, size_t aStride, size_t aBits)
		: mId(aId), mStride(aStride), mShift(0), mBits(aBits), mLimit(1<<mBits), mCount(0)
	{
		// adjust block shift
		for (mShift = 0; mShift < mBits; ++mShift)
		{
			if ((mStride << mShift) >= 1024)
				break;
		}

		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));

		mKey = static_cast<Key *>(malloc(mLimit * sizeof(Key)));
		memset(mKey, 0, mLimit * sizeof(Key));

		mPool = static_cast<void **>(malloc((mLimit >> mShift) * sizeof(void *)));
		memset(mPool, 0, (mLimit >> mShift) * sizeof(void *));

		if (mId)
			GetDatabases().Put(mId, this);
	}

	// destructor
	Untyped::~Untyped()
	{
		if (mId)
			GetDatabases().Delete(mId);

		free(mMap);
		free(mKey);
		for (size_t slot = 0; slot < mLimit >> mShift; ++slot)
			free(mPool[slot]);
		free(mPool);
	}

	// clear all records
	void Untyped::Clear(void)
	{
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));
		memset(mKey, 0, mLimit * sizeof(Key));
		for (size_t slot = 0; slot < mCount; ++slot)
			DeleteRecord(GetRecord(slot));
		mCount = 0;
	}

	// grow the database
	// (doubles the size)
	void Untyped::Grow(void)
	{
		// resize
		++mBits;
		mLimit = 1<<mBits;

		DebugPrint("Grow database %08x stride=%d shift=%d limit=%d count=%d\n", mId, mStride, mShift, mLimit, mCount);

		// reallocate map
		free(mMap);
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));

		// reallocate keys
		mKey = static_cast<Key *>(realloc(mKey, mLimit * sizeof(size_t)));
		memset(mKey + (mLimit >> 1), 0, (mLimit >> 1) * sizeof(size_t));

		// reallocate pools
		mPool = static_cast<void **>(realloc(mPool, (mLimit >> mShift) * sizeof(size_t)));
		memset(mPool + (mLimit >> (mShift + 1)), 0, (mLimit >> (mShift + 1)) * sizeof(void *));

		// rebuild hash
		for (size_t record = 0; record < mCount; ++record)
		{
			// get the record key
			size_t key = mKey[record];

			// convert key to a hash map index
			// (HACK: assume key is already a hash)
			size_t index = Probe(key);

			// insert the record key
			mMap[index] = record;
		}
	}

	// copy a source database
	void Untyped::Copy(const Untyped &aSource)
	{
		// clear existing pool
		Clear();

		// copy counts
		mBits = aSource.mBits;
		mLimit = aSource.mLimit;
		mCount = aSource.mCount;

		// copy map
		free(mMap);
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		memcpy(mMap, aSource.mMap, mLimit * 2 * sizeof(size_t));

		// copy keys
		free(mKey);
		mKey = static_cast<Key *>(malloc(mLimit * sizeof(size_t)));
		memcpy(mKey, aSource.mKey, mLimit * sizeof(size_t));

		// copy pools
		free(mPool);
		mPool = static_cast<void **>(malloc((mLimit >> mShift) * sizeof(size_t)));
		memset(mPool, 0, (mLimit >> mShift) * sizeof(void *));
		for (size_t slot = 0; slot < mCount; ++slot)
		{
			if ((slot & ((1 << mShift) - 1)) == 0)
			{
				mPool[slot >> mShift] = malloc((1 << mShift) * mStride);
			}
			CreateRecord(GetRecord(slot), aSource.GetRecord(slot));
		}
	}

	// find the record for a specified key
	const void *Untyped::Find(Key aKey) const
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is not empty...
		size_t slot = mMap[index];
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// check parent
		if (this != &parent)
			if (Key aParentKey = parent.Get(aKey))
				return Find(aParentKey);

		// not found
		return NULL;
	}

	// create or update a record for a specified key
	void Untyped::Put(Key aKey, const void *aValue)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is not empty...
		size_t slot = mMap[index];
		if (slot != EMPTY)
		{
			// update the record
			UpdateRecord(GetRecord(slot), aValue);
			return;
		}

		// grow if the database is full
		if (mCount >= mLimit)
			Grow();

		// add a new record
		slot = mCount++;
		index = Probe(aKey);
		mMap[index] = slot;
		mKey[slot] = aKey;
		if (mPool[slot >> mShift] == NULL)
			mPool[slot >> mShift] = malloc(mStride << mShift);
		CreateRecord(GetRecord(slot), aValue);
	}

	// get or create the record for a specified key
	void *Untyped::Open(Key aKey)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is not empty...
		size_t slot = mMap[index];
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// grow if the database is full
		if (mCount >= mLimit)
			Grow();

		// add a new record
		slot = mCount++;
		index = Probe(aKey);
		mMap[index] = slot;
		mKey[slot] = aKey;
		if (mPool[slot >> mShift] == NULL)
			mPool[slot >> mShift] = malloc(mStride << mShift);
		// check parent
		const void *source = NULL;
		if (this != &parent)
			if (Key aParentKey = parent.Get(aKey))
				source = Find(aParentKey);
		CreateRecord(GetRecord(slot), source);

		// return the record
		return GetRecord(slot);
	}

	// close a record once done
	void Untyped::Close(Key aKey)
	{
	}

	// allocate the record for a specified key
	void *Untyped::Alloc(Key aKey)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is not empty...
		size_t slot = mMap[index];
		if (slot != EMPTY)
		{
			// return the record
			assert(false);
			return NULL;
		}

		// grow if the database is full
		if (mCount >= mLimit)
			Grow();

		// add a new record
		slot = mCount++;
		index = Probe(aKey);
		mMap[index] = slot;
		mKey[slot] = aKey;
		if (mPool[slot >> mShift] == NULL)
			mPool[slot >> mShift] = malloc(mStride << mShift);

		// return the record
		return GetRecord(slot);
	}

	// delete a record for a specified key
	void Untyped::Delete(Key aKey)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is empty...
		size_t slot = mMap[index];
		if (slot == EMPTY)
		{
			// not found
			return;
		}

		// update record count
		--mCount;

		// if not the last record...
		if (slot < mCount)
		{
			// move the last record into the slot
			Key key = mKey[slot] = mKey[mCount];
			CreateRecord(GetRecord(slot), GetRecord(mCount));

			// update the entry
			for (size_t keyindex = Index(key); mMap[keyindex] != EMPTY; keyindex = Next(keyindex))
			{
				if (mMap[keyindex] == mCount)
				{
					mMap[keyindex] = slot;
					break;
				}
			}
		}

		// delete the last record
		DeleteRecord(GetRecord(mCount));
		mKey[mCount] = 0;

		// for each entry in the cluster...
		size_t nextindex = index;
		while (1)
		{
			// get the next entry
			nextindex = Next(nextindex);
			size_t nextslot = mMap[nextindex];

			// stop upon reaching the end of the cluster
			if (nextslot == EMPTY)
				break;

			// if the entry is out of place, and there is a place for it...
			Key key = mKey[nextslot];
			size_t keyindex = Index(key);
			if ((nextindex > index && (keyindex <= index || keyindex > nextindex)) ||
				(nextindex < index && (keyindex <= index && keyindex > nextindex)))
			{
				// move the entry
				mMap[index] = mMap[nextindex];
				index = nextindex;
			}
		}

		// clear the empty slot
		mMap[index] = EMPTY;
	}


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
		void AddConfigure(unsigned int aTagId, Entry aConfigure)
		{
			GetConfigureDB().Put(aTagId, aConfigure);
		}
		const Entry &GetConfigure(unsigned int aTagId)
		{
			return GetConfigureDB().Get(aTagId);
		}

		class TeamLoader
		{
		public:
			TeamLoader()
			{
				AddConfigure(0xa2fd7d0c /* "team" */, Entry(this, &TeamLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				unsigned int &team = Database::team.Open(aId);
				team = Hash(element->Attribute("name"));
				Database::team.Close(aId);
			}
		}
		teamloader;

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
		void AddActivate(unsigned int aDatabaseId, Entry aActivate)
		{
			GetActivate().Put(aDatabaseId, aActivate);
		}
		Typed<Entry> &GetDeactivate()
		{
			static Typed<Entry> ondeactivate;
			return ondeactivate;
		}
		void AddDeactivate(unsigned int aDatabaseId, Entry aDeactivate)
		{
			GetDeactivate().Put(aDatabaseId, aDeactivate);
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
	void Instantiate(unsigned int aInstanceId, unsigned int aTemplateId, float aAngle, Vector2 aPosition, Vector2 aVelocity, float aOmega)
	{
		// inherit components from template
//		Inherit(aInstanceId, aTemplateId);

		// set parent
		parent.Put(aInstanceId, aTemplateId);

		// objects default to owning themselves
		owner.Put(aInstanceId, aInstanceId);

		// create a new entity
		Entity *entity = new Entity(aInstanceId);
		entity->SetTransform(aAngle, aPosition);
		entity->SetVelocity(aVelocity);
		entity->SetOmega(aOmega);
		entity->Step();
		Database::entity.Put(aInstanceId, entity);

		// activate the instance identifier
		Activate(aInstanceId);
	}

	// instantiate a template (automatically-generated identifier)
	unsigned int Instantiate(unsigned int aTemplateId, float aAngle, Vector2 aPosition, Vector2 aVelocity, float aOmega)
	{
		// generate an instance identifier
		const unsigned int aInstanceTag = Entity::TakeId();
		const unsigned int aInstanceId = Hash(&aInstanceTag, sizeof(aInstanceTag), aTemplateId);

		// instantiate the template
		Instantiate(aInstanceId, aTemplateId, aAngle, aPosition, aVelocity, aOmega);

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

			// initialize entity
			Entity *entity = Database::entity.Get(aId);

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

			// initialize entity (HACK)
			entity->Init();

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

			// remove runtime components without templates
			if (Player *p = player.Get(aId))
			{
				delete p;
				player.Delete(aId);
			}
			if (Gunner *g = gunner.Get(aId))
			{
				delete g;
				gunner.Delete(aId);
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
	}

	// destroy the identifier
	void Destroy(unsigned int aId)
	{
		// deactivate
		Deactivate(aId);

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
		// deactivate all instances
		for (Typed<Entity *>::Iterator itor(&entity); itor.IsValid(); ++itor)
			Deactivate(itor.GetKey());

		// clear all registered databases
		for (Typed<Untyped *>::Iterator itor(&GetDatabases()); itor.IsValid(); ++itor)
			itor.GetValue()->Clear();
	}
}
