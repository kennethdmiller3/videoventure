#include "StdAfx.h"
#include "Damagable.h"
#include "Player.h"
#include "Team.h"

class CriticalTracker : public Updatable
{
public:
	int mCount;

public:
	CriticalTracker(unsigned int aId = 0)
		: Updatable(aId)
		, mCount(0)
	{
		SetAction(Updatable::Action(this, &CriticalTracker::Update));
	}

	void Update(float aStep)
	{
		// for each player...
		for (Database::Typed<Player *>::Iterator itor(&Database::player); itor.IsValid(); ++itor)
		{
			Player *player = itor.GetValue();
			if (Database::team.Get(player->GetId()) != mId)
				continue;

			Database::Delete(player->mAttach);
			player->Detach(player->mAttach);
			player->mLives = 0;
			player->Activate();
		}
	}

	void Track(int aAdd)
	{
		if (mCount == 0)
			Updatable::Deactivate();
		mCount += aAdd;
		if (mCount == 0)
			Updatable::Activate();
	}
};

namespace Database
{
	Typed<bool> criticalitem(0x26de6ef7 /* "criticalitem" */);

	Typed<CriticalTracker> criticaltracker(0x250bf3c4 /* "criticaltracker" */);

	namespace Loader
	{
		class CriticalItemLoader
		{
		public:
			CriticalItemLoader()
			{
				AddConfigure(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemLoader::Configure));
			}

			~CriticalItemLoader()
			{
				RemoveConfigure(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				bool &criticalitem = Database::criticalitem.Open(aId);
				element->QueryBoolAttribute("value", &criticalitem);
				Database::criticalitem.Close(aId);
			}
		}
		criticalitemloader;
	}

	namespace Initializer
	{
		class CriticalItemInitializer
		{
		public:
			CriticalItemInitializer()
			{
				AddPostActivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::PostActivate));
				AddPreDeactivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::PreDeactivate));
			}

			~CriticalItemInitializer()
			{
				RemovePostActivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::PostActivate));
				RemovePreDeactivate(0x26de6ef7 /* "criticalitem" */, Entry(this, &CriticalItemInitializer::PreDeactivate));
			}

			void PostActivate(unsigned int aId)
			{
				unsigned int team = Database::team.Get(aId);
				if (!Database::criticaltracker.Find(team))
					new (Database::criticaltracker.Alloc(team)) CriticalTracker(team);
				CriticalTracker &tracker = Database::criticaltracker.Open(team);
				tracker.Track(1);
				DebugPrint("critical item activate team=%0x8f count=%d\n", team, tracker.mCount); 
				Database::criticaltracker.Close(team);
			}

			void PreDeactivate(unsigned int aId)
			{
				unsigned int team = Database::team.Get(aId);
				CriticalTracker &tracker = Database::criticaltracker.Open(team);
				tracker.Track(-1);
				DebugPrint("critical item deactivate team=%0x8f count=%d\n", team, tracker.mCount); 
				Database::criticaltracker.Close(team);
			}
		}
		criticaliteminitializer;
	}
}
