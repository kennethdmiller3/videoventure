#include "StdAfx.h"
#include "Pickup.h"
#include "Entity.h"
#include "Collidable.h"
#include "Link.h"
#include "Team.h"
#include "Renderable.h"
#include "Resource.h"
#include "Damagable.h"
#include "Points.h"
#include "PlayerController.h"

#ifdef USE_POOL_ALLOCATOR

// pickup pool
static MemoryPool sPool(sizeof(Pickup));
void *Pickup::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Pickup::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<PickupTemplate> pickuptemplate(0x01ebaacb /* "pickuptemplate" */);
	Typed<Pickup *> pickup(0x6958f085 /* "pickup" */);

	Typed<Typed<Typed<LinkTemplate> > > pickuplink(0xaf3da629 /* "pickuplink" */);
	Typed<Typed<Typed<float> > > pickupresource(0x15fc7db5 /* "pickupresource" */);

	namespace Loader
	{
		static void PickupConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			PickupTemplate &pickup = Database::pickuptemplate.Open(aId);
			pickup.Configure(element, aId);
			Database::pickuptemplate.Close(aId);
		}
		Configure pickupconfigure(0x6958f085 /* "pickup" */, PickupConfigure);
	}

	namespace Initializer
	{
		static void PickupActivate(unsigned int aId)
		{
			const PickupTemplate &pickuptemplate = Database::pickuptemplate.Get(aId);
			Pickup *pickup = new Pickup(pickuptemplate, aId);
			Database::pickup.Put(aId, pickup);
		}
		Activate pickupactivate(0x01ebaacb /* "pickuptemplate" */, PickupActivate);

		static void PickupDeactivate(unsigned int aId)
		{
			if (Pickup *pickup = Database::pickup.Get(aId))
			{
				delete pickup;
				Database::pickup.Delete(aId);
			}
		}
		Deactivate pickupdeactivate(0x01ebaacb /* "pickuptemplate" */, PickupDeactivate);
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

bool PickupTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	if (const char *spawn = element->Attribute("spawnoncollect"))
		mSpawnOnCollect = Hash(spawn);
	if (const char *spawn = element->Attribute("switchoncollect"))
		mSwitchOnCollect = Hash(spawn);

	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0xa2fd7d0c /* "team" */:
			{
				if (const char *teamname = child->Attribute("name"))
				{
					unsigned int aTeamId = Hash(teamname);

					for (const tinyxml2::XMLElement *param = child->FirstChildElement(); param != NULL; param = param->NextSiblingElement())
					{
						switch (Hash(param->Value()))
						{
						case 0x0ddb0669 /* "link" */:
							if (const char *linkname = param->Attribute("name"))
							{
								unsigned int aSubId = Hash(linkname);

								Database::Typed<Database::Typed<LinkTemplate> > &teamlinks = Database::pickuplink.Open(aId);
								Database::Typed<LinkTemplate> &links = teamlinks.Open(aTeamId);
								LinkTemplate &link = links.Open(aSubId);
								link.Configure(param, aId, aSubId);
								links.Close(aSubId);
								teamlinks.Close(aTeamId);
								Database::pickuplink.Close(aId);
							}
							break;

						case 0x29df7ff5 /* "resource" */:
							if (const char *resourcename = param->Attribute("name"))
							{
								unsigned int aSubId = Hash(resourcename);

								Database::Typed<Database::Typed<float> > &teamresources = Database::pickupresource.Open(aId);
								Database::Typed<float> &resources = teamresources.Open(aTeamId);
								float &resource = resources.Open(aSubId);
								param->QueryFloatAttribute("add", &resource);
								resources.Close(aSubId);
								teamresources.Close(aTeamId);
								Database::pickupresource.Close(aId);
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
	Collidable::ContactSignal &signal = Database::collidablecontactadd.Open(mId);
	signal.Connect(this, &Pickup::Collide);
	Database::collidablecontactadd.Close(mId);
}

Pickup::~Pickup(void)
{
	Collidable::ContactSignal &signal = Database::collidablecontactadd.Open(mId);
	signal.Disconnect(this, &Pickup::Collide);
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
		// was used?
		bool wasUsed = false;

		// get team affiliation
		unsigned int aHitTeam = Database::team.Get(mHitId);

		if (Database::pickuplink.Find(mId))
			Database::Deactivate(mHitId);

		// for each pickup link...
#ifdef PICKUP_CASCADE_LINK_CHAIN
		const Database::Typed<Link *> &links = Database::link.Get(mHitId);
#endif
		for (Database::Typed<LinkTemplate>::Iterator itor(Database::pickuplink.Get(mId).Find(aHitTeam)); itor.IsValid(); ++itor)
		{
			// if the recipient supports the link...
			if (Database::linktemplate.Get(mHitId).Find(itor.GetKey()))
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
					linktemplate.mSecondary = chain ? link : prev;
					DebugPrint("%s link %08x: %s -> %s\n", Database::name.Get(mHitId).c_str(), name, Database::name.Get(prev).c_str(), Database::name.Get(link).c_str());

					// if the group is empty
					if (groupempty)
					{
						// stop chaining
						chain = false;
					}
					else
					{
						// push link template contents forward
						link = prev;
					}

					// close link template
					linktemplates.Close(name);
				}
#else
				// link name
				unsigned int name = itor.GetKey();

				// get baseline version
				unsigned int parent = Database::parent.Get(mHitId);
				const LinkTemplate &linktemplatebase = Database::linktemplate.Get(parent).Get(name);

				// merge templates (HACK)
				// TO DO: figure out how to determine template overrides
				const LinkTemplate &link = itor.GetValue();
				LinkTemplate merge(linktemplatebase);
				merge.mSecondary = link.mSecondary;
				merge.mOffset = link.mOffset * linktemplatebase.mOffset;

				// set link template based on link
				DebugPrint("%s link %08x: %s -> %s\n", Database::name.Get(mHitId).c_str(), name, Database::name.Get(linktemplates.Get(name).mSecondary).c_str(), Database::name.Get(link.mSecondary).c_str());
				linktemplates.Put(name, merge);
#endif

#ifdef PICKUP_CASCADE_LINK_CHAIN
				// close link templates
				Database::linktemplate.Close(mHitId);
#endif

				// was used
				wasUsed = true;
			}
		}

		// for each pickup resource...
		for (Database::Typed<float>::Iterator itor(Database::pickupresource.Get(mId).Find(aHitTeam)); itor.IsValid(); ++itor)
		{
			// if the recipient supports the resource...
			if (Resource *resource = Database::resource.Get(mHitId).Get(itor.GetKey()))
			{
				// amount to add
				const float add = itor.GetValue();

				// get the resource template
				const ResourceTemplate &resourcetemplate = Database::resourcetemplate.Get(mHitId).Get(itor.GetKey());

				// if not at the limit...
				if ((add > 0.0f && resource->GetValue() < resourcetemplate.mMaximum) ||
					(add < 0.0f && resource->GetValue() > 0.0f))
				{
				// add the resource
					resource->Add(mId, add);

					// was used
					wasUsed = true;
				}
			}
		}

		if (Database::pickuplink.Find(mId))
			Database::Activate(mHitId);

		if (Pickup *pickup = Database::pickup.Get(mId))
			pickup->Kill(mTime);

		// get hit owner
		unsigned int aOwnerId = Database::owner.Get(mHitId);

		if (!wasUsed)
		{
			// notify all source kill listeners
			Database::killsignal.Get(mHitId)(mHitId, mId);

			// notify all owner kill listeners
			Database::killsignal.Get(aOwnerId)(aOwnerId, mId);

			// notify all death listeners
			Database::deathsignal.Get(mId)(mId, mHitId);
		}

		Deactivate();
		delete this;
	}
};

#ifdef USE_POOL_ALLOCATOR
// kill update pool
static MemoryPool sKillPool(sizeof(PickupGrantUpdate));

void *PickupGrantUpdate::operator new(size_t aSize)
{
	return sKillPool.Alloc();
}
void PickupGrantUpdate::operator delete(void *aPtr)
{
	sKillPool.Free(aPtr);
}
#endif


void Pickup::Collide(unsigned int aId, unsigned int aHitId, float aFraction, const Vector2 &aContact, const Vector2 &aNormal)
{
	// do nothing if destroyed...
	if (mDestroy)
		return;
	assert(mId == aId);

	// get team affiliation
	unsigned int aHitTeam = Database::team.Get(aHitId);

	// for each pickup link...
	for (Database::Typed<LinkTemplate>::Iterator itor(Database::pickuplink.Get(mId).Find(aHitTeam)); itor.IsValid(); ++itor)
	{
		// if the recipient supports the link...
		if (Database::linktemplate.Get(aHitId).Find(itor.GetKey()))
		{
			// apply
			mDestroy = true;
			break;
		}
	}

	// for each pickup resource...
	for (Database::Typed<float>::Iterator itor(Database::pickupresource.Get(mId).Find(aHitTeam)); itor.IsValid(); ++itor)
	{
		// if the recipient supports the resource...
		if (Resource *resource = Database::resource.Get(aHitId).Get(itor.GetKey()))
		{
			// amount to add
			const float add = itor.GetValue();

			// get the resource template
			const ResourceTemplate &resourcetemplate = Database::resourcetemplate.Get(aHitId).Get(itor.GetKey());

			// if not at the limit...
			if ((add > 0.0f && resource->GetValue() < resourcetemplate.mMaximum) ||
				(add < 0.0f && resource->GetValue() > 0.0f))
		{
				// apply
			mDestroy = true;
			break;
			}
		}
	}

	// if it grants points...
	if (Database::points.Get(mId))
	{
		// if the recipient is player-controlled...
		if (Database::playercontroller.Find(aHitId))
		{
			mDestroy = true;
		}
	}

	if (mDestroy)
	{
		new PickupGrantUpdate(mId, aHitId, aFraction);
	}
}
