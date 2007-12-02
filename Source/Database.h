#pragma once

//#include <boost/pool/object_pool.hpp>

namespace Database
{
	// database key
	typedef unsigned int Key;

	// boost::object_pool<T> pool;

	// typed database
	template <typename T> class Typed : public stdext::hash_map<Key, T>
	{
	protected:
		const unsigned int id;
		const T nil;

	public:
		Typed(const char *name)
			: id(Hash(name)), nil()
		{
		}

		const T *Find(Key key) const
		{
			iterator itor = find(key);
			if (itor != end())
				return &itor->second;
			return NULL;
		}

		const T &Get(Key key) const
		{
			const_iterator itor = find(key);
			if (itor != end())
				return itor->second;
			return nil;
		}

		void Put(Key key, const T &value)
		{
			(*this)[key] = value;
		}

		void Delete(Key key)
		{
			erase(key);
		}
	};

	// parent identifier database
	extern Typed<unsigned int> parent;
}
