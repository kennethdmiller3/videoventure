#pragma once

#include "DatabaseUntyped.h"

namespace Database
{
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
		Typed(void)
			: Untyped(0, sizeof(T), 4)
		{
			CreateRecord(mNil);
		}

		Typed(unsigned int aId)
			: Untyped(aId, sizeof(T), 8)
		{
			CreateRecord(mNil);
		}

		Typed(unsigned int aId, unsigned int aBits)
			: Untyped(aId, sizeof(T), aBits)
		{
			CreateRecord(mNil);
		}

		Typed(unsigned int aId, unsigned int aBits, const T& aNil)
			: Untyped(aId, sizeof(T), aBits)
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
}
