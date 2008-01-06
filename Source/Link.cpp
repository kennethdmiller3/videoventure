#include "StdAfx.h"
#include "Link.h"
#include "Entity.h"

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
{
}

LinkTemplate::~LinkTemplate(void)
{
}

bool LinkTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x0ddb0669 /* "link" */)
		return false;

	if (const char *name = element->Attribute("name"))
		mSub = Hash(name);
	if (const char *secondary = element->Attribute("secondary"))
		mSecondary = Hash(secondary);

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
		}
	}
	return true;
}


Link::Link(void)
: Simulatable(0)
, mSecondary(0)
{
}

Link::Link(const LinkTemplate &aTemplate, unsigned int aId)
: Simulatable(aId)
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
	}
}

Link::~Link(void)
{
	if (mSecondary)
	{
		Database::Delete(mSecondary);
	}
}

void Link::Simulate(float aStep)
{
	if (mSecondary)
	{
		// get entities
		Entity *entity = Database::entity.Get(Simulatable::id);
		Entity *secondary = Database::entity.Get(mSecondary);

		// get template data
		const LinkTemplate &link = Database::linktemplate.Get(Simulatable::id).Get(mSub);

		// update secondary transform
		Matrix2 transform(link.mOffset * entity->GetTransform());
		secondary->Step();
		secondary->SetTransform(transform);
		secondary->SetVelocity(entity->GetVelocity());
	}
}
