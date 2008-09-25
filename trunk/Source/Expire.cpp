#include "StdAfx.h"
#include "Expire.h"

#include "Entity.h"
#include "Drawlist.h"
#include "Player.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// damagable pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Expire));
void *Expire::operator new(size_t aSize)
{
	return pool.malloc();
}
void Expire::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<ExpireTemplate> expiretemplate(0x40558d04 /* "expiretemplate" */);
	Typed<Expire *> expire(0x80459822 /* "expire" */);

	namespace Loader
	{
		class ExpireLoader
		{
		public:
			ExpireLoader()
			{
				AddConfigure(0x80459822 /* "expire" */, Entry(this, &ExpireLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				ExpireTemplate &expire = Database::expiretemplate.Open(aId);
				expire.Configure(element);
				Database::expiretemplate.Close(aId);
			}
		}
		expireloader;
	}

	namespace Initializer
	{
		class ExpireInitializer
		{
		public:
			ExpireInitializer()
			{
				AddActivate(0x40558d04 /* "expiretemplate" */, Entry(this, &ExpireInitializer::Activate));
				AddDeactivate(0x40558d04 /* "expiretemplate" */, Entry(this, &ExpireInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const ExpireTemplate &expiretemplate = Database::expiretemplate.Get(aId);
				Expire *expire = new Expire(expiretemplate, aId);
				Database::expire.Put(aId, expire);
				expire->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Expire *expire = Database::expire.Get(aId))
				{
					delete expire;
					Database::expire.Delete(aId);
				}
			}
		}
		expireinitializer;
	}
}


//
// EXPIRE TEMPLATE

// constructor
ExpireTemplate::ExpireTemplate(void)
: mTime(FLT_MAX)
, mSpawn(0)
, mSwitch(0)
, mReticule(false)
{
}

// destructor
ExpireTemplate::~ExpireTemplate(void)
{
}

// configure
bool ExpireTemplate::Configure(const TiXmlElement *element)
{
	element->QueryFloatAttribute("time", &mTime);
	if (const char *spawn = element->Attribute("spawnonexpire"))
		mSpawn = Hash(spawn);
	if (const char *spawn = element->Attribute("switchonexpire"))
		mSwitch = Hash(spawn);
	int reticule = mReticule;
	element->QueryIntAttribute("reticule", &reticule);
	mReticule = reticule != 0;
	return true;
}


//
// EXPIRE INSTANCE

// default constructor
Expire::Expire(void)
: Updatable(0)
, mTurn(INT_MAX)
, mFraction(0.0f)
{
}

// instance constructor
Expire::Expire(const ExpireTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mTurn(INT_MAX)
, mFraction(0.0f)
{
	// set action
	SetAction(Action(this, &Expire::Update));

	// expiration time
	float aTime = aTemplate.mTime;

	// if directed to the reticule...
	if (aTemplate.mReticule)
	{
		unsigned int owner =  Database::owner.Get(aId);
		if (owner && Database::player.Find(owner))
		{
			// get flight time to the aim point
			extern Vector2 camerapos[];
			extern float VIEW_SIZE;
			Vector2 aimdir(input[Input::AIM_HORIZONTAL], input[Input::AIM_VERTICAL]);
			Vector2 aimpos(camerapos[1] + aimdir * 120 * VIEW_SIZE / 320);
			Entity *entity = Database::entity.Get(aId);
			float aimtime = entity->GetVelocity().Dot(aimpos - entity->GetPosition()) / entity->GetVelocity().LengthSq();
			aTime = std::min(aTime, aimtime);

			// place a mark at the aim point
			Vector2 localpos = entity->GetTransform().Unrotate(entity->GetVelocity() * aTime);
			Database::Typed<float> &variables = Database::variable.Open(aId);
			variables.Put(0x8dfebaf1 /* "localaim" */ + 0, localpos.x);
			variables.Put(0x8dfebaf1 /* "localaim" */ + 1, localpos.y);
			variables.Put(0x8dfebaf1 /* "localaim" */ + 2, 0.0f);
			variables.Put(0x8dfebaf1 /* "localaim" */ + 3, 1.0f);
			Database::variable.Close(aId);
		}
	}

	if (aTime < FLT_MAX)
	{
		// turn and fraction to expire
		float turns = aTime * sim_rate;
		mTurn = xs_FloorToInt(turns + sim_fraction);
		mFraction = turns - mTurn + sim_fraction;
		mTurn += sim_turn;
	}

}

// destructor
Expire::~Expire(void)
{
}

// update
void Expire::Update(float aStep)
{
	// skip if not expired...
	float t = int(sim_turn - mTurn) + sim_fraction - mFraction;
	if (t < 0.0f)
		return;

	const ExpireTemplate &expire = Database::expiretemplate.Get(mId);

	// if spawning on expire...
	if (expire.mSpawn)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// rewind position to fraction
			entity->SetPosition(entity->GetInterpolatedPosition(1.0f - t));
			entity->SetAngle(entity->GetInterpolatedAngle(1.0f - t));

			// spawn template at the entity location
			Database::Instantiate(expire.mSpawn, Database::owner.Get(mId), mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
		}
	}

	// if switching on expire...
	if (expire.mSwitch)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// rewind position to fraction
			entity->SetPosition(entity->GetInterpolatedPosition(1.0f - t));
			entity->SetAngle(entity->GetInterpolatedAngle(1.0f - t));

			// change dynamic type
			Database::Switch(mId, expire.mSwitch);
		}
	}
	else
	{
		// delete the entity
		Database::Delete(mId);
	}
}