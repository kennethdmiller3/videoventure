#pragma once

#include "MemoryPool.h"

namespace Database
{
	// database key
	typedef unsigned int Key;

	// reference-counted memory pool
	class MemoryPoolRef : public MemoryPool
	{
	private:
		unsigned int mRef;

	private:
		~MemoryPoolRef()
		{
		}

	public:
		MemoryPoolRef(void)
			: MemoryPool(), mRef(1)
		{
		}

		MemoryPoolRef(unsigned int aSize, unsigned int aStart, unsigned int aGrow)
			: MemoryPool(aSize, aStart, aGrow), mRef(1)
		{
		}

		unsigned int AddRef(void)
		{
			return ++mRef;
		}

		unsigned int Release(void)
		{
			if (--mRef > 0)
				return mRef;
			delete this;
			return 0;
		}

#ifdef USE_POOL_ALLOCATOR
		void *operator new(size_t aSize);
		void operator delete(void *aPtr);
#endif
	};

	// untyped (core) database
	class GAME_API Untyped
	{
	protected:
		const unsigned int mId;

		static const unsigned int EMPTY = ~0U;

		MemoryPoolRef *mPool;	// (shared) memory pool

		unsigned int mBits;		// bit count
		unsigned int mLimit;	// maximum number of database records (1 << bit count)
		unsigned int mCount;	// current number of database records
		unsigned int mMask;		// map index mask
		unsigned int *mMap;		// map key to database records (2x maximum)
		Key *mKey;				// database record key pool
		void **mData;			// database record data pool
		void *mNil;				// database default record

	protected:
		void Alloc(void);
		void Free(void);
		void Grow(void);
		void Copy(const Untyped &aSource);

		inline unsigned int Index(Key aKey) const
		{
			// convert key to a hash map index
			// (HACK: assume key is already a hash)
			return ((aKey >> (mBits + 1)) ^ aKey) & mMask;
		}

		inline unsigned int Next(unsigned int aIndex) const
		{
			return (aIndex + 1) & mMask;
		}

		inline unsigned int FindIndex(Key aKey) const
		{
			unsigned int index = Index(aKey);

			// while the slot is not empty...
			unsigned int slot = mMap[index];
			while (slot != EMPTY && mKey[slot] != aKey)
			{
				index = Next(index);
				slot = mMap[index];
			}

			return index;
		}

		inline unsigned int FindSlot(Key aKey) const
		{
			unsigned int index = Index(aKey);

			// while the slot is not empty...
			unsigned int slot = mMap[index];
			while (slot != EMPTY && mKey[slot] != aKey)
			{
				index = Next(index);
				slot = mMap[index];
			}

			return slot;
		}

		void *AllocRecord(Key aKey)
		{
			unsigned int slot = mCount++;
			unsigned int index = FindIndex(aKey);
			_ASSERTE(index >= 0 && index < mLimit * 2);
			_ASSERTE(mMap[index] == EMPTY);
			mMap[index] = slot;
			_ASSERTE(slot >= 0 && slot < mCount);
			_ASSERTE(mKey[slot] == 0);
			mKey[slot] = aKey;
			_ASSERTE(mData[slot] == NULL);
			mData[slot] = mPool->Alloc();
			return memset(GetRecord(slot), 0, GetStride());
		}

		inline void *GetRecord(unsigned int aSlot) const
		{
			_ASSERTE(aSlot >= 0 && aSlot < mCount);
			_ASSERTE(mData[aSlot] != NULL);
			return mData[aSlot];
		}
		virtual void CreateRecord(void *aDest, const void *aSource = NULL)
		{
			if (aSource)
				memcpy(aDest, aSource, GetStride());
			else
				memset(aDest, 0, GetStride());
		}
		virtual void UpdateRecord(void *aDest, const void *aSource)
		{
			if (aSource)
				memcpy(aDest, aSource, GetStride());
			else
				memset(aDest, 0, GetStride());
		}
		virtual void DeleteRecord(void *aDest)
		{
		};

	public:
		Untyped(unsigned int aId, unsigned int aStride, unsigned int aBits);
		Untyped(const Untyped &aSource);
		virtual ~Untyped();

		const Untyped &operator=(const Untyped &aSource)
		{
			Copy(aSource);
			return *this;
		}

		void Clear();

		size_t GetStride(void) const
		{
			return mPool->GetSize();
		}
		size_t GetChunk(void) const
		{
			return mPool->GetGrow();
		}
		unsigned int GetLimit(void) const
		{
			return mLimit;
		}
		unsigned int GetCount(void) const
		{
			return mCount;
		}

		const void *Find(Key aKey) const;
		const void *Get(Key aKey) const;
		void Put(Key aKey, const void *aValue);
		void *Open(Key aKey);
		void Close(Key aKey);
		void *Alloc(Key aKey);
		void Delete(Key aKey);

		const void *GetDefault(void) const
		{
			return mNil;
		}

		void *OpenDefault(void)
		{
			return mNil;
		}

		void CloseDefault(void)
		{
		}

		class Iterator
		{
		protected:
			const Untyped *mDatabase;
			unsigned int mSlot;

		public:
			// default constructor
			Iterator(const Untyped *aDatabase, unsigned int aSlot = 0)
				: mDatabase(aDatabase), mSlot(aSlot)
			{
			}

			// copy constructor
			Iterator(const Iterator &aIterator)
				: mDatabase(aIterator.mDatabase), mSlot(aIterator.mSlot)
			{
			}

			// pre-increment
			const Iterator &operator++(void)
			{
				++mSlot;
				return *this;
			}
			
			// post-increment
			const Iterator operator++(int)
			{
				return Iterator(mDatabase, mSlot++);
			}

			// pre-decrement
			const Iterator &operator--(void)
			{
				--mSlot;
				return *this;
			}
			
			// post-decrement
			const Iterator operator--(int)
			{
				return Iterator(mDatabase, mSlot--);
			}

			// is the iterator valid?
			bool IsValid(void)
			{
				return mDatabase && mSlot >= 0 && mSlot < mDatabase->mCount;
			}

			// get the iterator slot
			unsigned int GetSlot(void)
			{
				return mSlot;
			}

			// get the iterator key
			Key GetKey(void)
			{
				return mDatabase->mKey[mSlot];
			}

			// get the iterator value
			void *GetValue(void)
			{
				return mDatabase->GetRecord(mSlot);
			}
		};
	};
}
