#include "StdAfx.h"
#include "Cancelable.h"

#include "Entity.h"
#include "Drawlist.h"
#include "Player.h"
#include "Damagable.h"
#include "Link.h"

#include "Renderable.h"


#ifdef USE_POOL_ALLOCATOR
// damagable pool
static MemoryPool sPool(sizeof(Cancelable));
void *Cancelable::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Cancelable::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif




//
// Tether rendering for Session 14

// burn the tether
class TetherBurn : public Updatable, public Renderable
{
public:
	Vector2 mSourcePos;		// position of source end
	unsigned int mOwnerId;	// damage owner
	float mDamage;			// damage to inflict on target
	int mCombo;				// combo level
	unsigned int mEnd;		// end turn

public:
#ifdef USE_POOL_ALLOCATOR
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	TetherBurn(unsigned int aId, unsigned int aSourceId, unsigned int aOwnerId, float aDamage, int aCombo)
		: Updatable(aId), mOwnerId(aOwnerId), mDamage(aDamage), mCombo(aCombo)
	{
		DebugPrint("%s <- %s\n", Database::name.Get(aId).c_str(), Database::name.Get(aSourceId).c_str());

		Renderable::mId = aId;

		Updatable::SetAction(Updatable::Action(this, &TetherBurn::Update));
		Updatable::Activate();

		Renderable::SetAction(Renderable::Action(this, &TetherBurn::Render));
		Renderable::Show();

		Entity *entity = Database::entity.Get(aId);
		Entity *source = Database::entity.Get(aSourceId);

		mSourcePos = source->GetPosition();
		mEnd = mStart + int(entity->GetPosition().Dist(mSourcePos) * sim_rate / 240.0f);
	}

	void Update(float aStep)
	{
		if (sim_turn >= mEnd)
		{
			// bump the hit combo counter
			int &combo = Database::hitcombo.Open(Updatable::mId);
			combo = std::max<int>(combo, mCombo + 1);
			Database::hitcombo.Close(Updatable::mId);

			if (Damagable *damagable = Database::damagable.Get(Updatable::mId))
				damagable->Damage(mOwnerId, mDamage);
			if (Cancelable *cancelable = Database::cancelable.Get(Updatable::mId))
				cancelable->Cancel(Updatable::mId, mOwnerId);
			Deactivate();
			delete this;
		}
	}

	void Render(unsigned int aId, float aParam, const Transform2 &aTransform)
	{
		const Vector2 aPos0 = aTransform.p;
		const Vector2 aPos1 = Lerp(mSourcePos, aPos0, float(sim_turn - mStart + sim_fraction - mFraction) / float(mEnd - mStart - mFraction));

		glLineWidth(SCREEN_HEIGHT / 240.0f);

		glBegin(GL_LINES);

		glColor4f(0.7f, 0.7f, 0.7f, 0.5f);
		glVertex2f(aPos0.x, aPos0.y);
		glVertex2f(aPos1.x, aPos1.y);

		glColor4f(0.2f, 0.2f, 0.2f, 0.5f);
		glVertex2f(aPos1.x, aPos1.y);
		glVertex2f(mSourcePos.x, mSourcePos.y);

		for (int i = 0; i < 8; ++i)
		{
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glVertex2f(aPos1.x, aPos1.y);
			glColor4f(1.0f, 0.0f, 1.0f, 0.25f);
			glVertex2f(aPos1.x + 16.0f * (Random::Float() - Random::Float()), aPos1.y + 16.0f * (Random::Float() - Random::Float()));
		}

		glEnd();

		glLineWidth(1);
	}
};

#ifdef USE_POOL_ALLOCATOR
// cancel update pool
static MemoryPool sTetherBurnPool(sizeof(TetherBurn));

void *TetherBurn::operator new(size_t aSize)
{
	return sTetherBurnPool.Alloc();
}
void TetherBurn::operator delete(void *aPtr)
{
	sTetherBurnPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<CancelableTemplate> cancelabletemplate(0x7ea1f4e5 /* "cancelabletemplate" */);
	Typed<Cancelable *> cancelable(0xc1804ae7 /* "cancelable" */);

	namespace Loader
	{
		static void CancelableConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			CancelableTemplate &cancelable = Database::cancelabletemplate.Open(aId);
			cancelable.Configure(element);
			Database::cancelabletemplate.Close(aId);
		}
		Configure cancelableconfigure(0xc1804ae7 /* "cancelable" */, CancelableConfigure);
	}

	namespace Initializer
	{
		static void CancelableActivate(unsigned int aId)
		{
			const CancelableTemplate &cancelabletemplate = Database::cancelabletemplate.Get(aId);
			Cancelable *cancelable = new Cancelable(cancelabletemplate, aId);
			Database::cancelable.Put(aId, cancelable);
		}
		Activate cancelableactivate(0x7ea1f4e5 /* "cancelabletemplate" */, CancelableActivate);

		static void CancelableDeactivate(unsigned int aId)
		{
			if (Cancelable *cancelable = Database::cancelable.Get(aId))
			{
				delete cancelable;
				Database::cancelable.Delete(aId);
			}
		}
		Deactivate cancelabledeactivate(0x7ea1f4e5 /* "cancelabletemplate" */, CancelableDeactivate);
	}
}


//
// CANCELABLE TEMPLATE

// constructor
CancelableTemplate::CancelableTemplate(void)
: mSpawn(0)
, mSwitch(0)
, mTethered(false)
, mBacklash(0)
{
}

// destructor
CancelableTemplate::~CancelableTemplate(void)
{
}

// configure
bool CancelableTemplate::Configure(const tinyxml2::XMLElement *element)
{
	if (const char *spawn = element->Attribute("spawnoncancel"))
		mSpawn = Hash(spawn);
	if (const char *spawn = element->Attribute("switchoncancel"))
		mSwitch = Hash(spawn);
	element->QueryBoolAttribute("tethered", &mTethered);
	element->QueryFloatAttribute("backlash", &mBacklash);
	return true;
}


class CancelableCancelUpdate : public Updatable
{
public:
	unsigned int mSourceId;

public:
#ifdef USE_POOL_ALLOCATOR
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	CancelableCancelUpdate(unsigned int aId, unsigned int aSourceId)
		: Updatable(aId), mSourceId(aSourceId)
	{
		Updatable::SetAction(Updatable::Action(this, &CancelableCancelUpdate::Update));
		Updatable::Activate();
	}

	void Update(float aStep)
	{
		if (Cancelable *cancelable = Database::cancelable.Get(Updatable::mId))
			cancelable->Cancel(Updatable::mId, mSourceId);
		Deactivate();
		delete this;
	}
};

#ifdef USE_POOL_ALLOCATOR
// cancel update pool
static MemoryPool sCancelPool(sizeof(CancelableCancelUpdate));

void *CancelableCancelUpdate::operator new(size_t aSize)
{
	return sCancelPool.Alloc();
}
void CancelableCancelUpdate::operator delete(void *aPtr)
{
	sCancelPool.Free(aPtr);
}
#endif

//
// CANCELABLE INSTANCE

// default constructor
Cancelable::Cancelable(void)
{
}

// instance constructor
Cancelable::Cancelable(const CancelableTemplate &aTemplate, unsigned int aId)
: mId(aId)
{
	if (aTemplate.mTethered)
	{
		for (unsigned int creator = Database::creator.Get(mId); creator != 0; creator = Database::backlink.Get(creator))
		{
			if (Database::damagable.Find(creator))
			{
				Damagable::DeathSignal &signal = Database::deathsignal.Open(creator);
				signal.Connect(this, &Cancelable::CreatorDeath);
				Database::deathsignal.Close(creator);
				break;
			}
		}
	}
}

// destructor
Cancelable::~Cancelable(void)
{
	const CancelableTemplate &cancelable = Database::cancelabletemplate.Get(mId);

	if (cancelable.mTethered)
	{
		for (unsigned int creator = Database::creator.Get(mId); creator != 0; creator = Database::backlink.Get(creator))
		{
			if (Database::damagable.Find(creator))
			{
				Damagable::DeathSignal &signal = Database::deathsignal.Open(creator);
				signal.Disconnect(this, &Cancelable::CreatorDeath);
				Database::deathsignal.Close(creator);
				break;
			}
		}
	}
}

// cancel
void Cancelable::Cancel(unsigned int aId, unsigned int aSourceId)
{
	const CancelableTemplate &cancelable = Database::cancelabletemplate.Get(mId);

	// set owner to source damage owner
	unsigned int aOwnerId = Database::owner.Get(aSourceId);
	Database::owner.Put(mId, aOwnerId);

	// bump the hit combo counter
	int &combo = Database::hitcombo.Open(mId);
	combo = std::max<int>(combo, Database::hitcombo.Get(aSourceId) + 1);
	Database::hitcombo.Close(mId);

	if (cancelable.mBacklash)
	{
		for (unsigned int creator = Database::creator.Get(aId); creator != 0; creator = Database::backlink.Get(creator))
		{
			if (Damagable *damagable = Database::damagable.Get(creator))
			{
				Damagable::DeathSignal &signal = Database::deathsignal.Open(creator);
				signal.Disconnect(this, &Cancelable::CreatorDeath);
				Database::deathsignal.Close(creator);

//				damagable->Damage(mId, cancelable.mBacklash);

				// burn tether from the cancelable to the creator
				new TetherBurn(creator, mId, aOwnerId, cancelable.mBacklash, combo);
				break;
			}
		}
	}

	// if spawning on cancelable...
	if (cancelable.mSpawn)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// spawn template at the entity location
			Database::Instantiate(cancelable.mSpawn, Database::owner.Get(mId), mId, entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
		}
	}

	// if switching on cancelable...
	if (cancelable.mSwitch)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// change dynamic type
			Database::Switch(mId, cancelable.mSwitch);
		}
	}
	else
	{
		// delete the entity
		Database::Delete(mId);
	}
}

void Cancelable::CreatorDeath(unsigned int aId, unsigned int aSourceId)
{
	// burn tether from the creator to the cancelable
	if (Database::entity.Get(mId))
	{
		if (Database::entity.Get(aId))
		{
			new TetherBurn(mId, aId, Database::owner.Get(aSourceId), 1.0f, Database::hitcombo.Get(aId) + 1);
		}
		else
		{
			new CancelableCancelUpdate(mId, aId);
		}
	}
}
