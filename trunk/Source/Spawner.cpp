#include "StdAfx.h"
#include "Spawner.h"
#include "Entity.h"
#include "Renderable.h"

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


namespace Database
{
	Typed<SpawnerTemplate> spawnertemplate(0x8b6ef6ad /* "spawnertemplate" */);
	Typed<Spawner *> spawner(0x4936726f /* "spawner" */);

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
: mOffset(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0))
, mInherit(1, 1)
, mVelocity(0, 0)
, mSpawn(0)
, mStart(0)
, mCycle(FLT_MAX)
, mTrack(false)
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
					mOffset = Matrix2(angle * float(M_PI) / 180.0f, mOffset.p);
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
: Updatable(0)
, mSpawn(0)
, mTimer(0)
{
}

// spawner instantiation constructor
Spawner::Spawner(const SpawnerTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mSpawn(0)
, mTimer(-aTemplate.mStart)
{
}

// spawner destructor
Spawner::~Spawner(void)
{
}

// spawner update
void Spawner::Update(float aStep)
{
	// get the spawner template
	const SpawnerTemplate &spawner = Database::spawnertemplate.Get(id);

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
	mTimer += aStep;

	// if the timer elapses...
	while (mTimer > 0.0f)
	{
		// get the spawner entity
		Entity *entity = Database::entity.Get(id);
		if (entity)
		{
			// instantiate the spawn entity
			Matrix2 transform(spawner.mOffset * entity->GetInterpolatedTransform(mTimer / aStep));
			Vector2 velocity(transform.Rotate(spawner.mInherit * transform.Unrotate(entity->GetVelocity()) + spawner.mVelocity));
			mSpawn = Database::Instantiate(spawner.mSpawn, Database::owner.Get(id), transform.Angle(), transform.p, velocity, entity->GetOmega());

			// if the spawner has a team...
			unsigned int team = Database::team.Get(id);
			if (team)
			{
				// propagate team to spawned item
				Database::team.Put(mSpawn, team);
			}

			// set fractional turn
			if (Renderable *renderable = Database::renderable.Get(mSpawn))
				renderable->SetFraction(mTimer / aStep);
		}

		// set the timer
		mTimer -= spawner.mCycle;
	}
}
