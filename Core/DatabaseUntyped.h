#pragma once

namespace Database
{
	// database aKey
	typedef unsigned int Key;

	// untyped (core) database
	class Untyped
	{
	protected:
		const unsigned int mId;

		static const size_t EMPTY = ~0U;
		size_t mStride;		// size of a database record
		size_t mShift;		// record block shift
		size_t mBits;		// bit count
		size_t mLimit;		// maximum number of database records (1 << bit count)
		size_t mCount;		// current number of database records
		size_t mMask;		// map index mask
		size_t *mMap;		// map key to database records (2x maximum)
		Key *mKey;			// database record key pool
		void **mPool;		// database record data pool
		void *mNil;			// database default record

	protected:
		void Alloc(void);
		void Free(void);
		void Grow(void);
		void Copy(const Untyped &aSource);

		inline size_t Index(Key aKey) const
		{
			// convert key to a hash map index
			// (HACK: assume key is already a hash)
			return ((aKey >> (mBits + 1)) ^ aKey) & mMask;
		}

		inline size_t Next(size_t aIndex) const
		{
			return (aIndex + 1) & mMask;
		}

		inline size_t FindIndex(Key aKey) const
		{
			size_t index = Index(aKey);

			// while the slot is not empty...
			size_t slot = mMap[index];
			while (slot != EMPTY && mKey[slot] != aKey)
			{
				index = Next(index);
				slot = mMap[index];
			}

			return index;
		}

		inline size_t FindSlot(Key aKey) const
		{
			size_t index = Index(aKey);

			// while the slot is not empty...
			size_t slot = mMap[index];
			while (slot != EMPTY && mKey[slot] != aKey)
			{
				index = Next(index);
				slot = mMap[index];
			}

			return slot;
		}

		inline void *AllocRecord(Key aKey)
		{
			size_t slot = mCount++;
			size_t index = FindIndex(aKey);
			mMap[index] = slot;
			mKey[slot] = aKey;
			if (mPool[slot >> mShift] == NULL)
				mPool[slot >> mShift] = malloc(mStride << mShift);
			return memset(GetRecord(slot), 0, mStride);
		}

		inline void *GetRecord(size_t aSlot) const
		{
			return static_cast<char *>(mPool[aSlot >> mShift]) + (aSlot & ((1 << mShift) - 1)) * mStride;
		}
		virtual void CreateRecord(void *aDest, const void *aSource = NULL)
		{
			if (aSource)
				memcpy(aDest, aSource, mStride);
			else
				memset(aDest, 0, mStride);
		}
		virtual void UpdateRecord(void *aDest, const void *aSource)
		{
			if (aSource)
				memcpy(aDest, aSource, mStride);
			else
				memset(aDest, 0, mStride);
		}
		virtual void DeleteRecord(void *aDest)
		{
		};

	public:
		Untyped(unsigned int aId, size_t aStride, size_t aBits);
		Untyped(const Untyped &aSource);
		virtual ~Untyped();

		const Untyped &operator=(const Untyped &aSource)
		{
			Copy(aSource);
			return *this;
		}

		void Clear();

		size_t GetStride(void)
		{
			return mStride;
		}
		size_t GetShift(void)
		{
			return mShift;
		}
		size_t GetBits(void)
		{
			return mBits;
		}
		size_t GetLimit(void)
		{
			return mLimit;
		}
		size_t GetCount(void)
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
			size_t mSlot;

		public:
			// default constructor
			Iterator(const Untyped *aDatabase, size_t aSlot = 0)
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
			size_t GetSlot(void)
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
