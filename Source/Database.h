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

	// typed database
	template <typename T> class Typed : public Untyped
	{
	protected:
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
		Typed(unsigned int aId = 0, const T& aNil = T())
			: Untyped(aId, sizeof(T), aId ? 8 : 4)
		{
			CreateRecord(mNil, &aNil);
		}

		Typed(const Typed &aDatabase)
			: Untyped(aDatabase.mId, sizeof(T), ~0U)
		{
			Copy(aDatabase);
		}

		virtual ~Typed()
		{
			Clear();
			DeleteRecord(mNil);
		}

		const T &GetDefault(void) const
		{
			return *static_cast<const T *>(Untyped::GetDefault());
		}

		T &OpenDefault(void)
		{
			return *static_cast<T *>(Untyped::OpenDefault());
		}

		void CloseDefault(void)
		{
			Untyped::CloseDefault();
		}

		const T *Find(Key aKey) const
		{
			return static_cast<const T *>(Untyped::Find(aKey));
		}

		const T &Get(Key aKey) const
		{
			return *static_cast<const T *>(Untyped::Get(aKey));
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
			DeleteRecord(mNil);
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
				return *static_cast<const T *>(Untyped::Iterator::GetValue());
			}
		};
	};

	namespace Loader
	{
		typedef fastdelegate::FastDelegate<void (unsigned int, const TiXmlElement *)> Entry;
		void AddConfigure(unsigned int aTagId, Entry aEntry);
		const Entry &GetConfigure(unsigned int aTagId);
	}

	namespace Initializer
	{
		typedef fastdelegate::FastDelegate<void (unsigned int)> Entry;
		void AddActivate(unsigned int aDatabaseId, Entry aEntry);
		void AddPostActivate(unsigned int aDatabaseId, Entry aEntry);
		void AddPreDeactivate(unsigned int aDatabaseId, Entry aEntry);
		void AddDeactivate(unsigned int aDatabaseId, Entry aEntry);
	}

	// get database of databases
	Typed<Untyped *> &GetDatabases();

	// name database
	extern Typed<std::string> name;

	// parent identifier database
	extern Typed<Key> parent;

	// owner identifier database
	extern Typed<Key> owner;

	// creator identifier database
	extern Typed<Key> creator;

	// instantiate a template
	void Instantiate(unsigned int aInstanceId, unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity = Vector2(0, 0), float aOmega = 0, bool aActivate = true);
	unsigned int Instantiate(unsigned int aTemplateId, unsigned int aOwnerId, unsigned int aCreatorId, float aAngle, Vector2 aPosition, Vector2 aVelocity = Vector2(0, 0), float aOmega = 0, bool aActivate = true);

	// inherit from a template
	void Inherit(unsigned int aInstanceId, unsigned int aTemplateId);

	// change an instance's type
	void Switch(unsigned int aInstanceId, unsigned int aTemplateId);

	// activate an identifier
	void Activate(unsigned int aId);
	
	// deactivate an identifier
	void Deactivate(unsigned int aId);

	// delete an identifier
	void Delete(unsigned int aid);

	// update the database system
	void Update(void);

	// clean up all databases
	void Cleanup(void);
}
