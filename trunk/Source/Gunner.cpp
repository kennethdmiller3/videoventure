#include "StdAfx.h"
#include "Gunner.h"
#include "Entity.h"
#include "Link.h"

namespace Database
{
	Typed<GunnerTemplate> gunnertemplate(0xe4c32aec /* "gunnertemplate" */);
	Typed<Gunner *> gunner(0xe063cbaa /* "gunner" */);

	namespace Loader
	{
		class GunnerLoader
		{
		public:
			GunnerLoader()
			{
				AddConfigure(0xe063cbaa /* "gunner" */, Entry(this, &GunnerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				GunnerTemplate &gunner = Database::gunnertemplate.Open(aId);
				gunner.Configure(element);
				Database::gunnertemplate.Close(aId);
			}
		}
		gunnerloader;
	}

	namespace Initializer
	{
		class GunnerInitializer
		{
		public:
			GunnerInitializer()
			{
				AddActivate(0xe4c32aec /* "gunnertemplate" */, Entry(this, &GunnerInitializer::Activate));
				AddDeactivate(0xe4c32aec /* "gunnertemplate" */, Entry(this, &GunnerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const GunnerTemplate &gunnertemplate = Database::gunnertemplate.Get(aId);
				Gunner *gunner = new Gunner(gunnertemplate, aId);
				Database::gunner.Put(aId, gunner);
			}

			void Deactivate(unsigned int aId)
			{
				if (Gunner *gunner = Database::gunner.Get(aId))
				{
					delete gunner;
					Database::gunner.Delete(aId);
				}
			}
		}
		gunnerinitializer;
	}
}


// Gunner Template Constructor
GunnerTemplate::GunnerTemplate(void)
: mFollowLength(32)
{
}

// Gunner Template Destructor
GunnerTemplate::~GunnerTemplate(void)
{
}

// Gunner Template Configure
bool GunnerTemplate::Configure(const TiXmlElement *element)
{
	if (Hash(element->Value()) != 0xe063cbaa /* "gunner" */)
		return false;

	element->QueryFloatAttribute("follow", &mFollowLength);
	return true;
}


// Gunner Constructor
Gunner::Gunner(const GunnerTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
{
	Entity *entity = Database::entity.Get(id);
	mTrackPos.push_back(entity->GetPosition());
	mTrackLength = 0.0f;
}

// Gunner Destructor
Gunner::~Gunner(void)
{
}

// Gunner Simulate
void Gunner::Simulate(float aStep)
{
	// get the owner
	unsigned int aOwnerId = Database::backlink.Get(id);

	// get the owner entity
	Entity *owner = Database::entity.Get(aOwnerId);

	// if the owner does not exist...
	if (!owner)
	{
		// self-destruct
		Database::Delete(id);
		return;
	}

	// gunner template
	const GunnerTemplate &gunner = Database::gunnertemplate.Get(id);

	// get owner movement
	float lastsegment = owner->GetPosition().Dist(mTrackPos.back());
	if (lastsegment > 0)
	{
		// accumulate movement distance
		mTrackLength += lastsegment;

		// while there is excess track length...
		while (mTrackLength > gunner.mFollowLength)
		{
			// get the excess length
			float excess = mTrackLength - gunner.mFollowLength;

			// get the first segment length
			float firstsegment = mTrackPos[1].Dist(mTrackPos[0]);

			// if the segment is longer than the excess...
			if (firstsegment > excess)
			{
				// shorten the segment
				mTrackPos[0] += excess / firstsegment * (mTrackPos[1] - mTrackPos[0]);
				mTrackLength -= excess;
				break;
			}
			else
			{
				// remove the segment
				mTrackLength -= firstsegment;
				mTrackPos.pop_front();
			}
		}

		// add new position
		mTrackPos.push_back(owner->GetPosition());
	}

	// move to new position
	Entity *entity = Database::entity.Get(id);
	entity->Step();
	entity->SetPosition(mTrackPos.front());
	entity->SetAngle(owner->GetAngle());
	entity->SetVelocity(owner->GetVelocity());	// <-- HACK!
	entity->SetOmega(owner->GetOmega());
}
