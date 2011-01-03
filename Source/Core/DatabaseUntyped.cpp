#include "StdAfx.h"

#include "DatabaseUntyped.h"

namespace Database
{
	//
	// UNTYPED DATABASE
	//

#ifdef USE_POOL_ALLOCATOR
	// pool of pools
	static MemoryPool &GetPoolPool(void)
	{
		static MemoryPool sPoolPool(sizeof(MemoryPoolRef), 256, 16);
		return sPoolPool;
	}
	void *MemoryPoolRef::operator new(size_t aSize)
	{
		return GetPoolPool().Alloc();
	}
	void MemoryPoolRef::operator delete(void *aPtr)
	{
		GetPoolPool().Free(aPtr);
	}
#endif

	// constructor
	Untyped::Untyped(unsigned int aId, size_t aStride, size_t aBits)
		: mId(aId), mBits(aBits), mLimit(1 << mBits), mCount(0), mMask((2 << mBits) - 1)
	{
		// if allocating...
		if (signed(aBits) >= 0)
		{
			// set up the memory pool
			mPool = new MemoryPoolRef(aStride, 0, std::max<size_t>(16, 1024 / aStride));

			// allocate memory
			Alloc();

			// fill with empty values
			memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));
			memset(mKey, 0, mLimit * sizeof(Key));
			memset(mData, 0, mLimit * sizeof(void *));
			memset(mNil, 0, GetStride());
		}
		else
		{
			// no pool (yet)
			mPool = NULL;

			// clear pointers
			mMap = NULL;
			mKey = NULL;
			mData = NULL;
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

		if (mPool)
		{
			mPool->Release();
			mPool = NULL;
		}
	}

	// allocate pools
	void Untyped::Alloc()
	{
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		mKey = static_cast<Key *>(malloc(mLimit * sizeof(Key)));
		mData = static_cast<void **>(malloc(mLimit * sizeof(void *)));
		mNil = malloc(GetStride());
	}

	// free pools
	void Untyped::Free()
	{
		if (mMap)
		{
			free(mMap);
			mMap = NULL;
		}
		if (mKey)
		{
			free(mKey);
			mKey = NULL;
		}
		if (mData)
		{
			free(mData);
			mData = NULL;
		}
		if (mNil)
		{
			free(mNil);
			mNil = NULL;
		}
	}

	// clear all records
	void Untyped::Clear(void)
	{
		for (size_t slot = 0; slot < mCount; ++slot)
		{
			void *record = GetRecord(slot);
			DeleteRecord(record);
			mPool->Free(record);
		}
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));
		memset(mKey, 0, mLimit * sizeof(Key));
		memset(mData, 0, mLimit * sizeof(void *));
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

		DebugPrint("Grow database this=%p id=%08x stride=%d chunk=%d limit=%d count=%d\n",
			this, mId, GetStride(), GetChunk(), GetLimit(), GetCount());

		// reallocate map
		free(mMap);
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));

		// reallocate keys
		mKey = static_cast<Key *>(realloc(mKey, mLimit * sizeof(size_t)));
		memset(mKey + mCount, 0, (mLimit - mCount) * sizeof(size_t));

		// reallocate data
		mData = static_cast<void **>(realloc(mData, mLimit * sizeof(void *)));
		memset(mData + mCount, 0, (mLimit - mCount) * sizeof(void *));

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

		// release pool
		if (mPool)
		{
			mPool->Release();
		}

		// use the source pool
		mPool = aSource.mPool;
		mPool->AddRef();

		// copy counts
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
		memset(mKey + mCount, 0, (mLimit - mCount) * sizeof(size_t));

		// copy data
		for (size_t slot = 0; slot < mCount; ++slot)
		{
			mData[slot] = mPool->Alloc();
			CreateRecord(GetRecord(slot), aSource.GetRecord(slot));
		}
		memset(mData + mCount, 0, (mLimit - mCount) * sizeof(void *));

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
		const void *source = mNil;
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

		// delete the record
		void *record = GetRecord(slot);
		DeleteRecord(record);
		mPool->Free(record);

		// update record count
		--mCount;

		// if not the last record...
		if (slot < mCount)
		{
			// move the last record into the vacant slot
			Key key = mKey[slot] = mKey[mCount];
			mData[slot] = mData[mCount];

			// update the map
			for (size_t keyindex = Index(key); mMap[keyindex] != EMPTY; keyindex = Next(keyindex))
			{
				if (mMap[keyindex] == mCount)
				{
					mMap[keyindex] = slot;
					break;
				}
			}
		}

		// clear the last record
		mData[mCount] = NULL;
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
