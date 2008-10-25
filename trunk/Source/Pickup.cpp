#include "StdAfx.h"
#include "Pickup.h"
#include "Link.h"
#include "Team.h"
#include "Renderable.h"
#include "Entity.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>


// pickup pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Pickup));
void *Pickup::operator new(size_t aSize)
{
	return pool.malloc();
}
void Pickup::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<Typed<Typed<LinkTemplate> > > pickupgrant(0x18e2ce2d /* "pickupgrant" */);
	Typed<PickupTemplate> pickuptemplate(0x01ebaacb /* "pickuptemplate" */);
	Typed<Pickup *> pickup(0x6958f085 /* "pickup" */);

	namespace Loader
	{
		class PickupLoader
		{
		public:
			PickupLoader()
			{
				AddConfigure(0x6958f085 /* "pickup" */, Entry(this, &PickupLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				PickupTemplate &pickup = Database::pickuptemplate.Open(aId);
				pickup.Configure(element, aId);
				Database::pickuptemplate.Close(aId);
			}
		}
		pickuploader;
	}

	namespace Initializer
	{
		class PickupInitializer
		{
		public:
			PickupInitializer()
			{
				AddActivate(0x01ebaacb /* "pickuptemplate" */, Entry(this, &PickupInitializer::Activate));
				AddDeactivate(0x01ebaacb /* "pickuptemplate" */, Entry(this, &PickupInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const PickupTemplate &pickuptemplate = Database::pickuptemplate.Get(aId);
				Pickup *pickup = new Pickup(pickuptemplate, aId);
				Database::pickup.Put(aId, pickup);
			}

			void Deactivate(unsigned int aId)
			{
				if (Pickup *pickup = Database::pickup.Get(aId))
				{
					delete pickup;
					Database::pickup.Delete(aId);
				}
			}
		}
		pickupinitializer;
	}
}


PickupTemplate::PickupTemplate(void)
: mSpawnOnCollect(0)
, mSwitchOnCollect(0)
{
}

PickupTemplate::~PickupTemplate(void)
{
}

bool PickupTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	if (const char *spawn = element->Attribute("spawnoncollect"))
		mSpawnOnCollect = Hash(spawn);
	if (const char *spawn = element->Attribute("switchoncollect"))
		mSwitchOnCollect = Hash(spawn);

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0xa2fd7d0c /* "team" */:
			{
				if (const char *teamname = child->Attribute("name"))
				{
					for (const TiXmlElement *param = child->FirstChildElement(); param != NULL; param = param->NextSiblingElement())
					{
						switch (Hash(param->Value()))
						{
						case 0x0ddb0669 /* "link" */:
							if (const char *linkname = param->Attribute("name"))
							{
								Database::Typed<Database::Typed<LinkTemplate> > &teamgrants = Database::pickupgrant.Open(aId);
								unsigned int aTeamId = Hash(teamname);
								Database::Typed<LinkTemplate> &grants = teamgrants.Open(aTeamId);
								unsigned int aSubId = Hash(linkname);
								LinkTemplate &grant = grants.Open(aSubId);
								grant.Configure(param, aId, aSubId);
								grants.Close(aSubId);
								teamgrants.Close(aTeamId);
								Database::pickupgrant.Close(aId);
							}
							break;
						}
					}
				}
			}
			break;
		}
	}

	return true;
}


Pickup::Pickup(void)
: mId(0), mDestroy(false)
{
}

Pickup::Pickup(const PickupTemplate &aTemplate, unsigned int aId)
: mId(aId), mDestroy(false)
{
	Database::Typed<Collidable::Listener> &listeners = Database::collidablecontactadd.Open(mId);
	Collidable::Listener &listener = listeners.Open(Database::Key(this));
	listener.bind(this, &Pickup::Collide);
	listeners.Close(Database::Key(this));
	Database::collidablecontactadd.Close(mId);
}

Pickup::~Pickup(void)
{
	Database::Typed<Collidable::Listener> &listeners = Database::collidablecontactadd.Open(mId);
	listeners.Delete(Database::Key(this));
	Database::collidablecontactadd.Close(mId);
}

void Pickup::Kill(float aFraction)
{
	const PickupTemplate &pickup = Database::pickuptemplate.Get(mId);

	// if spawning on pickup...
	if (pickup.mSpawnOnCollect)
	{
		// get the entity
		Entity *entity = Database::entity.Get(mId);
		if (entity)
		{
			// spawn template at entity location
			unsigned int spawnId = Database::Instantiate(pickup.mSpawnOnCollect, Database::owner.Get(mId), mId,
				entity->GetAngle(), entity->GetPosition(), entity->GetVelocity(), entity->GetOmega());
			if (Renderable *renderable = Database::renderable.Get(spawnId))
				renderable->SetFraction(aFraction);
		}
	}

	// if switching on pickup...
	if (pickup.mSwitchOnCollect)
	{
		// change dynamic type
		unsigned int aId = mId;
		Database::Switch(aId, pickup.mSwitchOnCollect);
		if (Renderable *renderable = Database::renderable.Get(aId))
			renderable->SetFraction(aFraction);
	}
	else
	{
		// delete the entity
		Database::Delete(mId);
	}

	return;
}


class PickupGrantUpdate : public Updatable
{
	float mTime;
	unsigned int mHitId;

public:
#ifdef USE_POOL_ALLOCATOR
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	PickupGrantUpdate(unsigned int aId, unsigned int aHitId, float aFraction)
		: Updatable(aId), mHitId(aHitId), mTime(aFraction)
	{
		SetAction(Action(this, &PickupGrantUpdate::Update));
		Activate();
	}

	void Update(float aStep)
	{
		// get team affiliation
		unsigned int aHitTeam = Database::team.Get(mHitId);

		// for each link template...
#ifdef PICKUP_CASCADE_LINK_CHAIN
		const Database::Typed<Link *> &links = Database::link.Get(mHitId);
#endif
		for (Database::Typed<LinkTemplate>::Iterator itor(Database::linktemplate.Find(mHitId)); itor.IsValid(); ++itor)
		{
			// get the grant link template
			const LinkTemplate &grant = Database::pickupgrant.Get(mId).Get(aHitTeam).Get(itor.GetKey());

			// if the pickup grants an item...
			if (grant.mSecondary)
			{
				// open link templates for the hit entity
				Database::Typed<LinkTemplate> &linktemplates = Database::linktemplate.Open(mHitId);

#ifdef PICKUP_CASCADE_LINK_CHAIN
				// cascade along link chain
				bool chain = true;
				for (unsigned int name = itor.GetKey(); name != 0; name = linktemplates.Get(name).mNext)
				{
					// get the link template
					LinkTemplate &linktemplate = linktemplates.Open(name);

					// save previous contents
					unsigned int prev = linktemplate.mSecondary;

					// get the link group
					unsigned int group = linktemplate.mGroup;
					bool groupempty = true;

					// get the corresponding link
					if (Link *link = links.Get(name))
					{
						// if the link is empty...
						if (!Database::entity.Find(link->GetSecondary()))
						{
							// clear value
							prev = 0U;
						}
					}

					// for each link...
					for (Database::Typed<Link *>::Iterator itor2(&links); itor2.IsValid(); ++itor2)
					{
						// if the link is not empty...
						Link *link = itor2.GetValue();
						if (Database::entity.Find(link->GetSecondary()))
						{
							// group is not empty
							if (groupempty && linktemplates.Get(itor2.GetKey()).mGroup == group)
							{
								groupempty = false;
							}
						}
					}

					// set link template based on chaining
					linktemplate.mSecondary = chain ? grant : prev;
					DebugPrint("%s link %08x: %s -> %s\n", Database::name.Get(mHitId).c_str(), name, Database::name.Get(prev).c_str(), Database::name.Get(grant).c_str());

					// if the group is empty
					if (groupempty)
					{
						// stop chaining
						chain = false;
					}
					else
					{
						// push link template contents forward
						grant = prev;
					}

					// close link template
					linktemplates.Close(name);
				}
#else
				// link name
				unsigned int name = itor.GetKey();

				// set link template based on grant
				DebugPrint("%s link %08x: %s -> %s\n", Database::name.Get(mHitId).c_str(), name, Database::name.Get(linktemplates.Get(name).mSecondary).c_str(), Database::name.Get(grant.mSecondary).c_str());
				linktemplates.Put(name, grant);
#endif

#ifdef PICKUP_CASCADE_LINK_CHAIN
				// close link templates
				Database::linktemplate.Close(mHitId);
#endif
			}
		}

		Database::Deactivate(mHitId);
		Database::Activate(mHitId);

		if (Pickup *pickup = Database::pickup.Get(mId))
			pickup->Kill(mTime);
		Deactivate();
		delete this;
	}
};

#ifdef USE_POOL_ALLOCATOR
// kill update pool
static boost::pool<boost::default_user_allocator_malloc_free> killpool(sizeof(PickupGrantUpdate));

void *PickupGrantUpdate::operator new(size_t aSize)
{
	return killpool.malloc();
}
void PickupGrantUpdate::operator delete(void *aPtr)
{
	killpool.free(aPtr);
}
#endif


void Pickup::Collide(unsigned int aId, unsigned int aHitId, float aFraction, const b2ContactPoint &aPoint)
{
	// do nothing if destroyed...
	if (mDestroy)
		return;
	assert(mId == aId);

	// get team affiliation
	unsigned int aHitTeam = Database::team.Get(aHitId);

	// for each link template...
	for (Database::Typed<LinkTemplate>::Iterator itor(Database::linktemplate.Find(aHitId)); itor.IsValid(); ++itor)
	{
		// get the grant link template
		const LinkTemplate &grant = Database::pickupgrant.Get(mId).Get(aHitTeam).Get(itor.GetKey());

		// if the pickup grants an item...
		if (grant.mSecondary)
		{
			mDestroy = true;
			break;
		}
	}

	if (mDestroy)
	{
		new PickupGrantUpdate(mId, aHitId, aFraction);
	}
}
