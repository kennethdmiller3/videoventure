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
	Untyped::Untyped(unsigned int aId, unsigned int aStride, unsigned int aBits)
		: mId(aId), mBits(aBits), mLimit(1U << mBits), mCount(0), mMask((2U << mBits) - 1U)
	{
		// if allocating...
		if (signed(aBits) >= 0)
		{
			// set up the memory pool
			mPool = new MemoryPoolRef(aStride, 0, std::max(16U, 1024U / aStride));

			// allocate memory
			Alloc();

			// fill with empty values
			memset(mMap, EMPTY, mLimit * 2 * sizeof(unsigned int));
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
		mMap = static_cast<unsigned int *>(malloc(mLimit * 2 * sizeof(unsigned int)));
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
		for (unsigned int slot = 0; slot < mCount; ++slot)
		{
			void *record = GetRecord(slot);
			DeleteRecord(record);
			mPool->Free(record);
		}
		memset(mMap, EMPTY, mLimit * 2 * sizeof(unsigned int));
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
		mMask = (2U << mBits) - 1U;
		mLimit = 1U << mBits;

		DebugPrint("Grow database this=%p id=%08x stride=%d chunk=%d limit=%d count=%d\n",
			this, mId, GetStride(), GetChunk(), GetLimit(), GetCount());

		// reallocate map
		free(mMap);
		mMap = static_cast<unsigned int *>(malloc(mLimit * 2 * sizeof(unsigned int)));
		memset(mMap, EMPTY, mLimit * 2 * sizeof(unsigned int));

		// reallocate keys
		mKey = static_cast<Key *>(realloc(mKey, mLimit * sizeof(Key)));
		memset(mKey + mCount, 0, (mLimit - mCount) * sizeof(Key));

		// reallocate data
		mData = static_cast<void **>(realloc(mData, mLimit * sizeof(void *)));
		memset(mData + mCount, 0, (mLimit - mCount) * sizeof(void *));

		// rebuild hash
		for (unsigned int record = 0; record < mCount; ++record)
		{
			// get the record key
			Key key = mKey[record];

			// convert key to a hash map index
			// (HACK: assume key is already a hash)
			unsigned int index = FindIndex(key);

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
		memcpy(mMap, aSource.mMap, mLimit * 2 * sizeof(unsigned int));

		// copy keys
		memcpy(mKey, aSource.mKey, mLimit * sizeof(Key));
		memset(mKey + mCount, 0, (mLimit - mCount) * sizeof(Key));

		// copy data
		for (unsigned int slot = 0; slot < mCount; ++slot)
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
		unsigned int slot = FindSlot(aKey);

		// if the slot is not empty...
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// check parent
		if (this != &parent && parent.GetCount())
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
		unsigned int slot = FindSlot(aKey);

		// if the slot is not empty...
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// check parent
		if (this != &parent && parent.GetCount())
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
		unsigned int slot = FindSlot(aKey);

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
		unsigned int slot = FindSlot(aKey);

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
		if (this != &parent && parent.GetCount())
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
		unsigned int slot = FindSlot(aKey);

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
		unsigned int index = FindIndex(aKey);

		// if the slot is empty...
		unsigned int slot = mMap[index];
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
			for (unsigned int keyindex = Index(key); mMap[keyindex] != EMPTY; keyindex = Next(keyindex))
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
		unsigned int nextindex = index;
		while (1)
		{
			// get the next entry
			nextindex = Next(nextindex);
			unsigned int nextslot = mMap[nextindex];

			// stop upon reaching the end of the cluster
			if (nextslot == EMPTY)
				break;

			// if the entry is out of place, and there is a place for it...
			Key key = mKey[nextslot];
			unsigned int keyindex = Index(key);
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
