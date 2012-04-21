#include "StdAfx.h"
#include "Spawner.h"
#include "Entity.h"
#include "Renderable.h"
#include "Team.h"


#ifdef USE_POOL_ALLOCATOR
// spawner pool
static MemoryPool sPool(sizeof(Spawner));
void *Spawner::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Spawner::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
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

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
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
, mScatter(0, Vector2(0, 0))
, mInherit(0, Vector2(1, 1))
, mVelocity(0, Vector2(0, 0))
, mVariance(0, Vector2(0, 0))
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
bool SpawnerTemplate::Configure(const tinyxml2::XMLElement *element)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x14c8d3ca /* "offset" */:
			{
				if (child->QueryFloatAttribute("angle", &mOffset.a) == tinyxml2::XML_SUCCESS)
					mOffset.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mOffset.p.x);
				child->QueryFloatAttribute("y", &mOffset.p.y);
			}
			break;

		case 0xcab7a341 /* "scatter" */:
			{
				if (child->QueryFloatAttribute("angle", &mScatter.a) == tinyxml2::XML_SUCCESS)
					mScatter.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mScatter.p.x);
				child->QueryFloatAttribute("y", &mScatter.p.y);
			}
			break;

		case 0xca04efe0 /* "inherit" */:
			{
				if (child->QueryFloatAttribute("angle", &mInherit.a) == tinyxml2::XML_SUCCESS)
					mInherit.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mInherit.p.x);
				child->QueryFloatAttribute("y", &mInherit.p.y);
			}
			break;

		case 0x32741c32 /* "velocity" */:
			{
				if (child->QueryFloatAttribute("angle", &mVelocity.a) == tinyxml2::XML_SUCCESS)
					mVelocity.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVelocity.p.x);
				child->QueryFloatAttribute("y", &mVelocity.p.y);
			}
			break;

		case 0x0dd0b0be /* "variance" */:
			{
				if (child->QueryFloatAttribute("angle", &mVariance.a) == tinyxml2::XML_SUCCESS)
					mVariance.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVariance.p.x);
				child->QueryFloatAttribute("y", &mVariance.p.y);
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
		if (!entity)
			return;

		// TO DO: consolidate this with similar spawn patterns (Graze, Weapon)

		// interpolated transform
		Transform2 transform(entity->GetInterpolatedTransform(mTimer / aStep));

		// apply transform offset
		transform = spawner.mOffset * transform;

		// apply transform scatter
		if (spawner.mScatter.a)
			transform.a += Random::Value(0.0f, spawner.mScatter.a);
		if (spawner.mScatter.p.x)
			transform.p.x += Random::Value(0.0f, spawner.mScatter.p.x);
		if (spawner.mScatter.p.y)
			transform.p.y += Random::Value(0.0f, spawner.mScatter.p.y);

		// get local velocity
		Transform2 velocity(entity->GetOmega(), transform.Unrotate(entity->GetVelocity()));

		// apply velocity inherit
		velocity.a *= spawner.mInherit.a;
		velocity.p.x *= spawner.mInherit.p.x;
		velocity.p.y *= spawner.mInherit.p.y;

		// apply velocity add
		velocity.a += spawner.mVelocity.a;
		velocity.p.x += spawner.mVelocity.p.x;
		velocity.p.y += spawner.mVelocity.p.y;

		// apply velocity variance
		if (spawner.mVariance.a)
			velocity.a += Random::Value(0.0f, spawner.mVariance.a);
		if (spawner.mVariance.p.x)
			velocity.p.x += Random::Value(0.0f, spawner.mVariance.p.x);
		if (spawner.mVariance.p.y)
			velocity.p.y += Random::Value(0.0f, spawner.mVariance.p.y);

		// get world velocity
		velocity.p = transform.Rotate(velocity.p);

		// apply fractional turn (HACK)
		transform.a += velocity.a * (aStep - mTimer);
		transform.p += velocity.p * (aStep - mTimer);

		// instantiate the spawn entity
		if (unsigned int spawnId = Database::Instantiate(spawner.mSpawn, Database::owner.Get(mId), mId, transform.a, transform.p, velocity.p, velocity.a, false))
		{
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

		// if tracking....
		if (spawner.mTrack)
		{
			// stop if out of slots
			if (mTrack >= spawner.mTrack)
				break;
		}
	}
}
