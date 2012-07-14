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
		static void GunnerConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			GunnerTemplate &gunner = Database::gunnertemplate.Open(aId);
			gunner.Configure(element);
			Database::gunnertemplate.Close(aId);
		}
		Configure gunnerconfigure(0xe063cbaa /* "gunner" */, GunnerConfigure);
	}

	namespace Initializer
	{
		static void GunnerActivate(unsigned int aId)
		{
			const GunnerTemplate &gunnertemplate = Database::gunnertemplate.Get(aId);
			Gunner *gunner = new Gunner(gunnertemplate, aId);
			Database::gunner.Put(aId, gunner);
			gunner->Activate();
		}
		Activate gunneractivate(0xe4c32aec /* "gunnertemplate" */, GunnerActivate);

		static void GunnerDeactivate(unsigned int aId)
		{
			if (Gunner *gunner = Database::gunner.Get(aId))
			{
				delete gunner;
				Database::gunner.Delete(aId);
			}
		}
		Deactivate gunnerdeactivate(0xe4c32aec /* "gunnertemplate" */, GunnerDeactivate);
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
bool GunnerTemplate::Configure(const tinyxml2::XMLElement *element)
{
	element->QueryFloatAttribute("follow", &mFollowLength);
	return true;
}


// Gunner Constructor
Gunner::Gunner(const GunnerTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
{
	SetAction(Action(this, &Gunner::Update));

	Entity *entity = Database::entity.Get(mId);
#ifdef GUNNER_TRACK_DEQUE
	mTrackPos.push_back(entity->GetPosition());
	mTrackPos.push_back(entity->GetPosition());
#else
	mTrackCount = xs_CeilToInt(aTemplate.mFollowLength/GUNNER_TRACK_GRANULARITY) + 1;
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

// Gunner Update
void Gunner::Update(float aStep)
{
	// get the owner
	unsigned int aOwnerId = Database::backlink.Get(mId);

	// get the owner entity
	Entity *owner = Database::entity.Get(aOwnerId);

	// if the owner does not exist...
	if (!owner)
	{
		// self-destruct
		Database::Delete(mId);
		return;
	}

	// gunner template
	const GunnerTemplate &gunner = Database::gunnertemplate.Get(mId);

	// get owner movement
	const Vector2 &posP = owner->GetPosition();
#ifdef GUNNER_TRACK_DEQUE
	const Vector2 &posL0 = mTrackPos.back();
#else
	const Vector2 &posL0 = mTrackPos[mTrackLast];
#endif
	float movement = posP.DistSq(posL0);

	// if the owner has moved...
	if (movement > FLT_EPSILON)
	{
#ifdef GUNNER_TRACK_DEQUE
		// get the last segment
		const Vector2 &posL1 = mTrackPos[mTrackPos.size()-2];
		float lastsegment = posL0.Dist(posL1);

		// if the last segment isn't long enough...
		if (lastsegment < GUNNER_TRACK_GRANULARITY)
		{
			// replace the last segment
			mTrackPos.pop_back();
			mTrackLength -= lastsegment;
		}

		// add new position
		mTrackPos.push_back(posP);
		mTrackLength += posP.Dist(mTrackPos[mTrackPos.size()-2]);
#else
		// get the last segment
		int mTrackPrev = (mTrackLast > 0) ? (mTrackLast - 1) : (mTrackCount - 1);
		const Vector2 &posL1 = mTrackPos[mTrackPrev];
		float lastsegment = posL0.Dist(posL1);

		// if the last segment is long enough...
		if (lastsegment >= GUNNER_TRACK_GRANULARITY)
		{
			// start a new segment
			mTrackPrev = mTrackLast;
			mTrackLast = (mTrackLast < mTrackCount - 1) ? (mTrackLast + 1) : 0;
		}
		else
		{
			// replace the last segment
			mTrackLength -= lastsegment;
		}

		// add new position
		mTrackPos[mTrackLast] = posP;
		mTrackLength += posP.Dist(mTrackPos[mTrackPrev]);
#endif

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
	}

	// move to new position
	Entity *entity = Database::entity.Get(mId);
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
