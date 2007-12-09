#include "StdAfx.h"
#include "Spawner.h"
#include "Entity.h"

namespace Database
{
	Typed<SpawnerTemplate> spawnertemplate("spawnertemplate");
	Typed<Spawner *> spawner("spawner");
}


// spawner template constructor
SpawnerTemplate::SpawnerTemplate(void)
: mOffset(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0)), mVelocity(0, 0), mSpawn(0), mStart(0), mCycle(0), mTrack(false)
{
}

// spawner template destructor
SpawnerTemplate::~SpawnerTemplate(void)
{
}

// spawner template configure
bool SpawnerTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x4936726f /* "spawner" */)
		return false;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x14c8d3ca /* "offset" */:
			{
				child->QueryFloatAttribute("x", &mOffset.p.x);
				child->QueryFloatAttribute("y", &mOffset.p.y);
				float angle = 0.0f;
				if (child->QueryFloatAttribute("angle", &angle) == TIXML_SUCCESS)
					mOffset = Matrix2(angle * float(M_PI) / 180.0f, mOffset.p);
			}
			break;

		case 0x32741c32 /* "velocity" */:
			{
				child->QueryFloatAttribute("x", &mVelocity.x);
				child->QueryFloatAttribute("y", &mVelocity.y);
			}
			break;

		case 0x3a224d98 /* "spawn" */:
			{
				if (const char *spawn = child->Attribute("name"))
					mSpawn = Hash(spawn);
				child->QueryFloatAttribute("start", &mStart);
				child->QueryFloatAttribute("cycle", &mCycle);
				int track = mTrack;
				child->QueryIntAttribute("track", &track);
				mTrack = track != 0;
			}
			break;
		}
	}

	return true;
}


// spawner default constructor
Spawner::Spawner(void)
: Simulatable(0)
, mSpawn(0)
, mTimer(0)
{
}

// spawner instantiation constructor
Spawner::Spawner(const SpawnerTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
, mSpawn(0)
, mTimer(aTemplate.mStart)
{
}

// spawner destructor
Spawner::~Spawner(void)
{
}

// spawner simulation
void Spawner::Simulate(float aStep)
{
	// get the spawner template
	const SpawnerTemplate &spawner = Database::spawnertemplate.Get(Simulatable::id);

	// if tracking the spawned item...
	if (spawner.mTrack)
	{
		// if the spawned item is still alive...
		if (Database::entity.Get(mSpawn))
		{
			// wait
			return;
		}

		// else if the spawned item just expired...
		else if (mSpawn)
		{
			// clear the spawned item
			mSpawn = 0;
		}
	}

	// advance the timer
	mTimer -= aStep;

	// if the timer elapses...
	if (mTimer <= 0.0f)
	{
		// get the spawner entity
		Entity *entity = Database::entity.Get(Simulatable::id);
		if (entity)
		{
			// instantiate the spawn entity
			Matrix2 transform(spawner.mOffset * entity->GetTransform());
			mSpawn = Database::Instantiate(spawner.mSpawn, transform.Angle(), transform.p, entity->GetVelocity() + transform.Rotate(spawner.mVelocity));

			// set the timer
			mTimer += spawner.mCycle;
		}
	}
}
