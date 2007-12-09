#pragma once

namespace Database
{
	// database aKey
	typedef unsigned int Key;

	// boost::object_pool<T> pool;

	// typed database
	template <typename T> class Typed : public stdext::hash_map<Key, T>
	{
	protected:
		const unsigned int mId;
		const T mNil;

	public:
		Typed(const char *aName)
			: mId(Hash(aName)), mNil()
		{
		}

		const T *Find(Key aKey) const
		{
			const_iterator itor = find(aKey);
			if (itor != end())
				return &itor->second;
			return NULL;
		}

		const T &Get(Key aKey) const
		{
			const_iterator itor = find(aKey);
			if (itor != end())
				return itor->second;
			return mNil;
		}

		void Put(Key aKey, const T &aValue)
		{
			(*this)[aKey] = aValue;
		}

		T &Open(Key aKey)
		{
			return (*this)[aKey];
		}

		void Close(Key aKey)
		{
		}

		void Delete(Key aKey)
		{
			erase(aKey);
		}
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
}
