#include "StdAfx.h"
#include "Spawner.h"
#include "Entity.h"
#include "Renderable.h"
#include "Team.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// spawner pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Spawner));
void *Spawner::operator new(size_t aSize)
{
	return pool.malloc();
}
void Spawner::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


class SpawnerTracker
{
public:
	unsigned int mId;

	SpawnerTracker(unsigned int aId = 0)
		: mId(aId)
	{
		if (Spawner *spawner = Database::spawner.Get(mId))
			spawner->Track(1);
	}

	SpawnerTracker(const SpawnerTracker &aSource)
		: mId(aSource.mId)
	{
		if (Spawner *spawner = Database::spawner.Get(mId))
			spawner->Track(1);
	}

	~SpawnerTracker()
	{
		if (Spawner *spawner = Database::spawner.Get(mId))
			spawner->Track(-1);
	}

	const SpawnerTracker &operator=(const SpawnerTracker &aSource)
	{
		if (Spawner *spawner = Database::spawner.Get(mId))
			spawner->Track(-1);
		if (Spawner *spawner = Database::spawner.Get(mId))
			spawner->Track(1);
		return *this;
	}
};


namespace Database
{
	Typed<SpawnerTemplate> spawnertemplate(0x8b6ef6ad /* "spawnertemplate" */);
	Typed<Spawner *> spawner(0x4936726f /* "spawner" */);
	Typed<SpawnerTracker> spawnertracker(0x9eefed29 /* "spawnertracker" */);

	namespace Loader
	{
		class SpawnerLoader
		{
		public:
			SpawnerLoader()
			{
				AddConfigure(0x4936726f /* "spawner" */, Entry(this, &SpawnerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				SpawnerTemplate &spawner = Database::spawnertemplate.Open(aId);
				spawner.Configure(element);
				Database::spawnertemplate.Close(aId);
			}
		}
		spawnerloader;
	}

	namespace Initializer
	{
		class SpawnerInitializer
		{
		public:
			SpawnerInitializer()
			{
				AddActivate(0x8b6ef6ad /* "spawnertemplate" */, Entry(this, &SpawnerInitializer::Activate));
				AddDeactivate(0x8b6ef6ad /* "spawnertemplate" */, Entry(this, &SpawnerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const SpawnerTemplate &spawnertemplate = Database::spawnertemplate.Get(aId);
				Spawner *spawner = new Spawner(spawnertemplate, aId);
				Database::spawner.Put(aId, spawner);
				spawner->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Spawner *spawner = Database::spawner.Get(aId))
				{
					delete spawner;
					Database::spawner.Delete(aId);
				}
			}
		}
		spawnerinitializer;
	}
}


// spawner template constructor
SpawnerTemplate::SpawnerTemplate(void)
: mOffset(0, Vector2(0, 0))
, mInherit(1, 1)
, mVelocity(0, 0)
, mSpawn(0)
, mStart(0)
, mCycle(FLT_MAX)
, mTrack(0)
{
}

// spawner template destructor
SpawnerTemplate::~SpawnerTemplate(void)
{
}

// spawner template configure
bool SpawnerTemplate::Configure(const TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x4936726f /* "spawner" */)
		return false;

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
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
					mOffset = Transform2(angle * float(M_PI) / 180.0f, mOffset.p);
			}
			break;

		case 0xca04efe0 /* "inherit" */:
			{
				child->QueryFloatAttribute("x", &mInherit.x);
				child->QueryFloatAttribute("y", &mInherit.y);
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
				child->QueryIntAttribute("track", &mTrack);
			}
			break;
		}
	}

	return true;
}


// spawner default constructor
Spawner::Spawner(void)
: Updatable(0)
, mTrack(0)
, mTimer(0)
{
	SetAction(Action(this, &Spawner::Update));
}

// spawner instantiation constructor
Spawner::Spawner(const SpawnerTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mTrack(0)
, mTimer(-aTemplate.mStart)
{
	SetAction(Action(this, &Spawner::Update));
}

// spawner destructor
Spawner::~Spawner(void)
{
	// remove listeners
}

// spawner update
void Spawner::Update(float aStep)
{
	// get the spawner template
	const SpawnerTemplate &spawner = Database::spawnertemplate.Get(mId);

	// skip if limit reached
	if (spawner.mTrack && mTrack >= spawner.mTrack)
		return;

	// advance the timer
	mTimer += aStep;

	// if the timer elapses...
	while (mTimer > 0.0f)
	{
		// get the spawner entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// instantiate the spawn entity
			Transform2 transform(spawner.mOffset * entity->GetInterpolatedTransform(mTimer / aStep));
			Vector2 velocity(transform.Rotate(spawner.mInherit * transform.Unrotate(entity->GetVelocity()) + spawner.mVelocity));
			unsigned int spawnId = Database::Instantiate(spawner.mSpawn, Database::owner.Get(mId), mId, transform.Angle(), transform.p, velocity, entity->GetOmega(), false);

			// if the spawner has a team...
			unsigned int team = Database::team.Get(mId);
			if (team)
			{
				// propagate team to spawned item
				Database::team.Put(spawnId, team);
			}

			// activate
			Database::Activate(spawnId);

			// set fractional turn
			if (Renderable *renderable = Database::renderable.Get(spawnId))
				renderable->SetFraction(mTimer / aStep);

			// if tracking....
			if (spawner.mTrack)
			{
				// add a tracker
				Database::spawnertracker.Put(spawnId, SpawnerTracker(mId));
			}
		}

		// set the timer
		mTimer -= spawner.mCycle;
	}
}
