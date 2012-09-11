#pragma once

#include "Task.h"

// forward declaration
class Controller;
class Brain;

class Behavior : public Task
{
protected:
	unsigned int mId;
	Controller *mController;

public:
	Behavior(unsigned int aId, Controller *aController)
		: mId(aId)
		, mController(aController)
	{
	}

	virtual ~Behavior()
	{
	}
};

namespace BehaviorDatabase
{
	namespace Loader
	{
		typedef unsigned int (* Entry)(unsigned int, const tinyxml2::XMLElement *);

		class GAME_API Configure
		{
		private:
			unsigned int mTagId;	// tag hash id for the configure
			Entry mPrev;			// entry that this replaced

		public:
			static Database::Typed<Entry> &GetDB();
			Configure(unsigned int aTagId, Entry aEntry);
			~Configure();
			static const Entry &Get(unsigned int aTagId);
		};
	}

	namespace Initializer
	{
		class GAME_API Activate
		{
		public:
			typedef Behavior * (* Entry)(unsigned int, Controller *);

		private:
			unsigned int mDatabaseId;
			Entry mPrev;

		public:
			static Database::Typed<Entry> &GetDB();
			Activate(unsigned int aDatabaseId, Entry aEntry);
			~Activate();
			static const Entry &Get(unsigned int aDatabaseId);
		};


		class GAME_API Deactivate
		{
		public:
			typedef void (* Entry)(unsigned int);

		private:
			unsigned int mDatabaseId;
			Entry mPrev;

		public:
			static Database::Typed<Entry> &GetDB();
			Deactivate(unsigned int aDatabaseId, Entry aEntry);
			~Deactivate();
			static const Entry &Get(unsigned int aDatabaseId);
		};
	}
}
