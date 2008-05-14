#include "StdAfx.h"
#include "Gunner.h"
#include "Entity.h"
#include "Link.h"

static const float GUNNER_TRACK_GRANULARITY = 1.0f;

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
				gunner->Activate();
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
#ifdef GUNNER_TRACK_DEQUE
	mTrackPos.push_back(entity->GetPosition());
#else
	mTrackCount = xs_CeilToInt(aTemplate.mFollowLength/GUNNER_TRACK_GRANULARITY);
	mTrackPos = new Vector2[mTrackCount];
	mTrackFirst = mTrackLast = 0;
	mTrackPos[0] = entity->GetPosition();
#endif
	mTrackLength = 0.0f;
}

// Gunner Destructor
Gunner::~Gunner(void)
{
#ifndef GUNNER_TRACK_DEQUE
	delete[] mTrackPos;
#endif
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
#ifdef GUNNER_TRACK_DEQUE
	float lastsegment = owner->GetPosition().Dist(mTrackPos.back());
#else
	float lastsegment = owner->GetPosition().Dist(mTrackPos[mTrackLast]);
#endif
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
#ifdef GUNNER_TRACK_DEQUE
			Vector2 &pos0 = mTrackPos[0];
			const Vector2 &pos1 = mTrackPos[1];
#else
			size_t mTrackNext = (mTrackFirst < mTrackCount - 1) ? (mTrackFirst + 1) : 0;
			Vector2 &pos0 = mTrackPos[mTrackFirst];
			const Vector2 &pos1 = mTrackPos[mTrackNext];
#endif
			float firstsegment = pos0.Dist(pos1);

			// if the segment is longer than the excess...
			if (firstsegment > excess)
			{
				// shorten the segment
				pos0 += excess / firstsegment * (pos1 - pos0);
				mTrackLength -= excess;
				break;
			}
			else
			{
				// remove the segment
				mTrackLength -= firstsegment;
#ifdef GUNNER_TRACK_DEQUE
				mTrackPos.pop_front();
#else
				mTrackFirst = mTrackNext;
#endif
			}
		}

#ifdef GUNNER_TRACK_DEQUE
		// replace last segment if shorter than the granularity
		if (mTrackPos.back().Dist(mTrackPos[mTrackPos.size()-2]) < GUNNER_TRACK_GRANULARITY)
			mTrackPos.pop_back();

		// add new position
		mTrackPos.push_back(owner->GetPosition());
#else
		// add a new segment if longer than the granularity
		if (mTrackPos[mTrackLast].Dist(mTrackPos[(mTrackLast > 0) ? (mTrackLast - 1) : (mTrackCount - 1)]) >= GUNNER_TRACK_GRANULARITY)
			mTrackLast = (mTrackLast < mTrackCount - 1) ? (mTrackLast + 1) : 0;

		// add new position
		mTrackPos[mTrackLast] = owner->GetPosition();
#endif
	}

	// move to new position
	Entity *entity = Database::entity.Get(id);
	entity->Step();
#ifdef GUNNER_TRACK_DEQUE
	entity->SetPosition(mTrackPos.front());
#else
	entity->SetPosition(mTrackPos[mTrackFirst]);
#endif
	entity->SetAngle(owner->GetAngle());
	entity->SetVelocity(owner->GetVelocity());	// <-- HACK!
	entity->SetOmega(owner->GetOmega());
}
