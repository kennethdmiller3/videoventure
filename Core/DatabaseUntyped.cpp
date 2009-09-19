#include "StdAfx.h"

#include "DatabaseUntyped.h"

namespace Database
{
	//
	// UNTYPED DATABASE
	//

	// constructor
	Untyped::Untyped(unsigned int aId, size_t aStride, size_t aBits)
		: mId(aId), mStride(aStride), mShift(0), mBits(aBits), mLimit(1 << mBits), mCount(0), mMask((2 << mBits) - 1)
	{
		// if allocating...
		if (signed(aBits) >= 0)
		{
			// adjust block shift
			for (mShift = 0; mShift < mBits; ++mShift)
			{
				if ((mStride << mShift) >= 1024)
					break;
			}

			// allocate memory
			Alloc();

			// fill with empty values
			memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));
			memset(mKey, 0, mLimit * sizeof(Key));
			memset(mPool, 0, (mLimit >> mShift) * sizeof(void *));
			memset(mNil, 0, mStride);
		}
		else
		{
			// clear pointers
			mMap = NULL;
			mKey = NULL;
			mPool = NULL;
			mNil = NULL;
		}

		if (mId)
			GetDatabases().Put(mId, this);
	}

	// destructor
	Untyped::~Untyped()
	{
		if (mId)
			GetDatabases().Delete(mId);

		Free();
	}

	// allocate pools
	void Untyped::Alloc()
	{
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		mKey = static_cast<Key *>(malloc(mLimit * sizeof(Key)));
		mPool = static_cast<void **>(malloc((mLimit >> mShift) * sizeof(void *)));
		mNil = malloc(mStride);
	}

	// free pools
	void Untyped::Free()
	{
		if (mMap)
			free(mMap);
		if (mKey)
			free(mKey);
		if (mPool)
		{
			for (size_t slot = 0; slot < mLimit >> mShift; ++slot)
				free(mPool[slot]);
			free(mPool);
		}
		if (mNil)
			free(mNil);
	}

	// clear all records
	void Untyped::Clear(void)
	{
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));
		memset(mKey, 0, mLimit * sizeof(Key));
		for (size_t slot = 0; slot < mCount; ++slot)
			DeleteRecord(GetRecord(slot));
		for (size_t slot = 0; slot < mLimit >> mShift; ++slot)
			free(mPool[slot]);
		memset(mPool, 0, (mLimit >> mShift) * sizeof(void *));
		mCount = 0;
	}

	// grow the database
	// (doubles the size)
	void Untyped::Grow(void)
	{
		// resize
		++mBits;
		mMask = (2 << mBits) - 1;
		mLimit = 1 << mBits;

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
			size_t index = FindIndex(key);

			// insert the record key
			mMap[index] = record;
		}
	}

	// copy a source database
	void Untyped::Copy(const Untyped &aSource)
	{
		// free existing arrays
		Free();

		// copy counts
		mShift = aSource.mShift;
		mBits = aSource.mBits;
		mMask = aSource.mMask;
		mLimit = aSource.mLimit;
		mCount = aSource.mCount;

		// create new arrays
		Alloc();

		// copy map
		memcpy(mMap, aSource.mMap, mLimit * 2 * sizeof(size_t));

		// copy keys
		memcpy(mKey, aSource.mKey, mLimit * sizeof(size_t));

		// copy pools
		memset(mPool, 0, (mLimit >> mShift) * sizeof(void *));
		for (size_t slot = 0; slot < mCount; ++slot)
		{
			if ((slot & ((1 << mShift) - 1)) == 0)
			{
				mPool[slot >> mShift] = malloc(mStride << mShift);
			}
			CreateRecord(GetRecord(slot), aSource.GetRecord(slot));
		}

		// copy default
		CreateRecord(mNil, aSource.mNil);
	}

	// find the record for a specified key
	const void *Untyped::Find(Key aKey) const
	{
		// convert key to a slot
		// (HACK: assume key is already a hash)
		size_t slot = FindSlot(aKey);

		// if the slot is not empty...
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

	// get the record for a specified key (or default if not found)
	const void *Untyped::Get(Key aKey) const
	{
		// convert key to a slot
		// (HACK: assume key is already a hash)
		size_t slot = FindSlot(aKey);

		// if the slot is not empty...
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// check parent
		if (this != &parent)
			if (Key aParentKey = parent.Get(aKey))
				return Get(aParentKey);

		// not found
		return mNil;
	}

	// create or update a record for a specified key
	void Untyped::Put(Key aKey, const void *aValue)
	{
		// convert key to a slot
		// (HACK: assume key is already a hash)
		size_t slot = FindSlot(aKey);

		// if the slot is not empty...
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
		void *record = AllocRecord(aKey);
		CreateRecord(record, aValue);
	}

	// get or create the record for a specified key
	void *Untyped::Open(Key aKey)
	{
		// convert key to a slot
		// (HACK: assume key is already a hash)
		size_t slot = FindSlot(aKey);

		// if the slot is not empty...
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// grow if the database is full
		if (mCount >= mLimit)
			Grow();

		// add a new record
		void *record = AllocRecord(aKey);

		// check parent
		const void *source = NULL;
		if (this != &parent)
			if (Key aParentKey = parent.Get(aKey))
				source = Find(aParentKey);
		CreateRecord(record, source);

		// return the record
		return record;
	}

	// close a record once done
	void Untyped::Close(Key aKey)
	{
	}

	// allocate the record for a specified key
	void *Untyped::Alloc(Key aKey)
	{
		// convert key to a slot
		// (HACK: assume key is already a hash)
		size_t slot = FindSlot(aKey);

		// if the slot is not empty...
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
		void *record = AllocRecord(aKey);

		// return the record
		return record;
	}

	// delete a record for a specified key
	void Untyped::Delete(Key aKey)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = FindIndex(aKey);

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
			DeleteRecord(GetRecord(slot));
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
}
