#include "StdAfx.h"
#include "Link.h"
#include "Entity.h"
#include "Collidable.h"

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// link pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Link));
void *Link::operator new(size_t aSize)
{
	return pool.malloc();
}
void Link::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<Typed<LinkTemplate> > linktemplate(0x801f01af /* "linktemplate" */);
	Typed<Typed<Link *> > link(0x0ddb0669 /* "link" */);
	Typed<unsigned int> backlink(0xe3736e9a /* "backlink" */);

	namespace Loader
	{
		class LinkLoader
		{
		public:
			LinkLoader()
			{
				AddConfigure(0x0ddb0669 /* "link" */, Entry(this, &LinkLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Database::Typed<LinkTemplate> &links = Database::linktemplate.Open(aId);
				unsigned int aSubId = Hash(element->Attribute("name"));
				LinkTemplate &link = links.Open(aSubId);
				link.Configure(element);
				links.Close(aSubId);
				Database::linktemplate.Close(aId);
			}
		}
		linkloader;
	}

	namespace Initializer
	{
		class LinkInitializer
		{
		public:
			LinkInitializer()
			{
				AddActivate(0x801f01af /* "linktemplate" */, Entry(this, &LinkInitializer::Activate));
				AddDeactivate(0x801f01af /* "linktemplate" */, Entry(this, &LinkInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				Typed<Link *> &links = Database::link.Open(aId);

				// for each link template...
				for (Typed<LinkTemplate>::Iterator itor(Database::linktemplate.Find(aId)); itor.IsValid(); ++itor)
				{
					// create the link
					const LinkTemplate &linktemplate = itor.GetValue();
					Link *link = new Link(linktemplate, aId);
					links.Put(itor.GetKey(), link);
				}

				Database::link.Close(aId);
			}

			void Deactivate(unsigned int aId)
			{
				for (Typed<Link *>::Iterator itor(Database::link.Find(aId)); itor.IsValid(); ++itor)
				{
					delete itor.GetValue();
				}
				Database::link.Delete(aId);
			}
		}
		linkinitializer;
	}
}


LinkTemplate::LinkTemplate(void)
: mOffset(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0))
, mSub(0)
, mSecondary(0)
, mUpdateAngle(true)
, mUpdatePosition(true)
{
}

LinkTemplate::~LinkTemplate(void)
{
}

bool LinkTemplate::Configure(const TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x0ddb0669 /* "link" */)
		return false;

	if (const char *name = element->Attribute("name"))
		mSub = Hash(name);
	if (const char *secondary = element->Attribute("secondary"))
		mSecondary = Hash(secondary);

	int updateangle = mUpdateAngle;
	element->QueryIntAttribute("updateangle", &updateangle);
	mUpdateAngle = updateangle != 0;

	int updateposition = mUpdatePosition;
	element->QueryIntAttribute("updateposition", &updateposition);
	mUpdatePosition = updateposition != 0;

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
		}
	}
	return true;
}


Link::Link(void)
: Updatable(0)
, mSecondary(0)
{
}

Link::Link(const LinkTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mSub(aTemplate.mSub)
, mSecondary(0)
{
	if (aTemplate.mSecondary)
	{
		// get the source entity
		Entity *entity = Database::entity.Get(aId);

		// instantiate the linked template
		Matrix2 transform(aTemplate.mOffset * entity->GetTransform());
		mSecondary = Database::Instantiate(aTemplate.mSecondary,
			transform.Angle(), transform.p, entity->GetVelocity());

		// create a backlink
		Database::backlink.Put(mSecondary, aId);

		// propagate ownership
		if (const unsigned int *aOwnerId = Database::owner.Find(aId))
			Database::owner.Put(mSecondary, *aOwnerId);

		// if not updating angle or position...
		if (!aTemplate.mUpdateAngle && !aTemplate.mUpdatePosition)
		{
			// disable udpate
			Deactivate();
		}

		// if linking two collidables...
		if (Database::collidabletemplate.Find(aId) &&
			Database::collidabletemplate.Find(mSecondary))
		{
			// disable update
			Deactivate();

			// add a revolute joint to the linked template (HACK)
			CollidableTemplate &collidable = Database::collidabletemplate.Open(mSecondary);
			collidable.joints.push_back(CollidableTemplate::JointTemplate());
			CollidableTemplate::JointTemplate &jointtemplate = collidable.joints.back();
			jointtemplate.name1 = aId;
			jointtemplate.body1 = 0xea90e208 /* "main" */;
			jointtemplate.name2 = mSecondary;
			jointtemplate.body2 = 0xea90e208 /* "main" */;
			collidable.revolutes.push_back(b2RevoluteJointDef());
			b2RevoluteJointDef &joint = collidable.revolutes.back();
			joint.userData = &jointtemplate;
			joint.lowerAngle = 0.0f;
			joint.upperAngle = 0.0f;
			joint.enableLimit = true;
			Database::collidabletemplate.Close(mSecondary);
		}
	}
}

Link::~Link(void)
{
	if (mSecondary)
	{
		Database::Delete(mSecondary);
	}
}

void Link::Update(float aStep)
{
	if (mSecondary)
	{
		// get entities
		Entity *entity = Database::entity.Get(id);
		Entity *secondary = Database::entity.Get(mSecondary);

		// get template data
		const LinkTemplate &link = Database::linktemplate.Get(id).Get(mSub);

		// update secondary transform
		Matrix2 transform(link.mOffset * entity->GetTransform());
		secondary->Step();
		if (link.mUpdateAngle)
		{
			secondary->SetAngle(transform.Angle());
		}
		if (link.mUpdatePosition)
		{
			secondary->SetPosition(transform.p);
			secondary->SetVelocity(entity->GetVelocity());
		}
	}
}
