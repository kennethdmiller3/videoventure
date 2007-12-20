#pragma once

namespace Database
{
	// database aKey
	typedef unsigned int Key;

	// boost::object_pool<T> pool;

	// untyped (core) database
	class Untyped
	{
	protected:
		const unsigned int mId;

		static const size_t SHIFT = 8;
		static const size_t EMPTY = ~0U;
		static const size_t DELETED = ~1U;
		size_t mStride;		// size of a database record
		int mBits;			// bit count
		size_t mLimit;		// maximum number of database records (1 << bit count)
		size_t mCount;		// current number of database records
		size_t *mMap;		// map key to database records (2x maximum)
		Key *mKey;			// database record key pool
		void **mPool;		// database record data pool

	protected:
		void Grow(void);
		void Copy(const Untyped &aSource);

		inline size_t Untyped::Index(Key aKey) const
		{
			// convert key to a hash map index
			// (HACK: assume key is already a hash)
			size_t shift = mBits + 1;
			size_t mask = (1 << shift) - 1;
			return ((aKey >> shift) ^ aKey) & mask;
		}

		inline size_t Untyped::Next(size_t aIndex) const
		{
			size_t shift = mBits + 1;
			size_t mask = (1 << shift) - 1;
			return (aIndex + 1) & mask;
		}

		inline size_t Untyped::Probe(Key aKey) const
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

		// implemented in typed databases
		void *GetRecord(size_t aSlot) const
		{
			return static_cast<char *>(mPool[aSlot >> SHIFT]) + (aSlot & ((1 << SHIFT) - 1)) * mStride;
		}
		virtual void CreateRecord(void *aDest, const void *aSource = NULL) = 0;
		virtual void UpdateRecord(void *aDest, const void *aSource) = 0;
		virtual void DeleteRecord(void *aDest) = 0;

	public:
		Untyped(unsigned int mId, size_t aStride);
		virtual ~Untyped();

		const Untyped &operator=(const Untyped &aSource)
		{
			Copy(aSource);
			return *this;
		}

		void Clear();

		const void *Find(Key aKey) const;
		void Put(Key aKey, const void *aValue);
		void *Open(Key aKey);
		void Close(Key aKey);
		void Delete(Key aKey);

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

			// is the iterator valid?
			bool IsValid(void)
			{
				return mDatabase && mSlot >= 0 && mSlot < mDatabase->mCount;
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

	// typed database
	template <typename T> class Typed : public Untyped
	{
	protected:
		const T mNil;

		virtual void CreateRecord(void *aDest, const void *aSource = NULL)
		{
			if (aSource)
				new (aDest) T(*static_cast<const T *>(aSource));
			else
				new (aDest) T;
		}
		virtual void UpdateRecord(void *aDest, const void *aSource)
		{
			*static_cast<T *>(aDest) = *static_cast<const T *>(aSource);
		}
		virtual void DeleteRecord(void *aDest)
		{
			static_cast<T *>(aDest)->~T();
		}

	public:
		Typed(const char *aName = "")
			: Untyped(Hash(aName), sizeof(T)), mNil()
		{
		}

		Typed(const Typed &aDatabase)
			: Untyped(aDatabase.mId, sizeof(T)), mNil(aDatabase.mNil)
		{
			Copy(aDatabase);
		}

		virtual ~Typed()
		{
			Clear();
		}

		const T *Find(Key aKey) const
		{
			return static_cast<const T *>(Untyped::Find(aKey));
		}

		const T &Get(Key aKey) const
		{
			const T *t = static_cast<const T *>(Untyped::Find(aKey));
			return t ? *t : mNil;
		}

		void Put(Key aKey, const T &aValue)
		{
			Untyped::Put(aKey, &aValue);
		}

		T &Open(Key aKey)
		{
			return *static_cast<T *>(Untyped::Open(aKey));
		}

		void Close(Key aKey)
		{
			Untyped::Close(aKey);
		}

		void Delete(Key aKey)
		{
			Untyped::Delete(aKey);
		}

		const Typed &operator=(const Typed &aSource)
		{
			Copy(aSource);
			return *this;
		}

		class Iterator : public Untyped::Iterator
		{
		public:
			Iterator(const Typed *aDatabase, size_t aSlot = 0)
				: Untyped::Iterator(aDatabase, aSlot)
			{
			}

			Iterator(const Iterator &aIterator)
				: Untyped::Iterator(aIterator)
			{
			}

			// get the iterator value
			const T &GetValue(void)
			{
				const T *t = static_cast<const T *>(Untyped::Iterator::GetValue());
				return t ? *t : static_cast<const Typed *>(mDatabase)->mNil;
			}
		};
	};

	// parent identifier database
	extern Typed<unsigned int> parent;

	// instantiate a template
	unsigned int Instantiate(unsigned int aTemplateId, float aAngle, Vector2 aPosition, Vector2 aVelocity);

	// inherit from a template
	void Inherit(unsigned int aInstanceId, unsigned int aTemplateId);

	// activate an identifier
	void Activate(unsigned int aId);
	
	// deactivate an identifier
	void Deactivate(unsigned int aId);

	// delete an identifier
	void Delete(unsigned int aid);

	// update the database system
	void Update(void);
}
