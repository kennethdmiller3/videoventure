#include "StdAfx.h"
#include "Link.h"
#include "Entity.h"
#include "Collidable.h"
#include "PhysicsRevoluteJoint.h"


#ifdef USE_POOL_ALLOCATOR
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
				link.Configure(element, aId, aSubId);
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

					// create the link
					Link *link = new Link(linktemplate, aId);
					links.Put(itor.GetKey(), link);

					// if linking two collidables...
					unsigned int aSecondary = link->GetSecondary();
					if (Database::collidabletemplate.Find(aId) &&
						Database::collidabletemplate.Find(aSecondary))
					{
						// if updating position
						if (linktemplate.mUpdatePosition)
						{
							// get revolute joints for the link target
							Database::Typed<b2RevoluteJointDef> &joints = Database::revolutejointdef.Open(aSecondary);
							
							// configure the joint definition
							b2RevoluteJointDef &def = joints.Open(aId);
							def.bodyA = reinterpret_cast<b2Body *>(aId);
							def.bodyB = reinterpret_cast<b2Body *>(aSecondary);
							def.localAnchorA.Set(linktemplate.mOffset.p.x, linktemplate.mOffset.p.y);
							def.localAnchorB.Set(0, 0);
							def.referenceAngle = linktemplate.mOffset.Angle();
							if (linktemplate.mUpdateAngle)
							{
								def.lowerAngle = 0.0f;
								def.upperAngle = 0.0f;
								def.enableLimit = true;
							}
							joints.Close(aId);

							Database::revolutejointdef.Close(aSecondary);
						}
					}
					// else if updating angle or position...
					else if (linktemplate.mUpdateAngle || linktemplate.mUpdatePosition)
					{
						// activate link update
						link->Activate();
					}
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
: mOffset(0, Vector2(0, 0))
, mSub(0)
, mSecondary(0)
, mType(0)
, mUpdateAngle(true)
, mUpdatePosition(true)
, mDeleteSecondary(true)
{
}

LinkTemplate::~LinkTemplate(void)
{
}

bool LinkTemplate::Configure(const TiXmlElement *element, unsigned int aId, unsigned int aSubId)
{
	// set sub-identifier
	mSub = aSubId;

	// set linked template
	if (const char *secondary = element->Attribute("secondary"))
		mSecondary = Hash(secondary);

	// set type identifier
	if (const char *type = element->Attribute("type"))
		mType = Hash(type);

	// update linked angle?
	int updateangle = mUpdateAngle;
	element->QueryIntAttribute("updateangle", &updateangle);
	mUpdateAngle = updateangle != 0;

	// update linked position?
	int updateposition = mUpdatePosition;
	element->QueryIntAttribute("updateposition", &updateposition);
	mUpdatePosition = updateposition != 0;

	// delete linked secondary?
	int deletesecondary = mDeleteSecondary;
	element->QueryIntAttribute("deletesecondary", &deletesecondary);
	mDeleteSecondary = deletesecondary != 0;

	// custom template identifier (if any)
	unsigned int custom = 0U;

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

		default:
			{
				const char *value = child->Value();
				const Database::Loader::Entry &configure = Database::Loader::GetConfigure(Hash(value));
				if (configure)
				{
					if (!custom)
					{
						// create a customized template
						DebugPrint("Link custom template \"%s:%s\"\n", Database::name.Get(aId).c_str(), element->Attribute("name"));
						custom = Hash(":", aId);
						custom = Hash(element->Attribute("name"), custom);
						Database::Inherit(custom, mSecondary);
						mSecondary = custom;
					}
					configure(mSecondary, child);
				}
				else
				{
					DebugPrint("Unrecognized tag \"%s\"\n", value);
				}
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
	SetAction(Action(this, &Link::Update));
}

Link::Link(const LinkTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mSub(aTemplate.mSub)
, mSecondary(aTemplate.mSecondary)
, mOffset(aTemplate.mOffset)
, mUpdateAngle(aTemplate.mUpdateAngle)
, mUpdatePosition(aTemplate.mUpdatePosition)
, mDeleteSecondary(aTemplate.mDeleteSecondary)
{
	SetAction(Action(this, &Link::Update));

	// get the source entity
	Entity *entity = Database::entity.Get(mId);

	// if not already instantiated...
	if (!Database::entity.Get(aTemplate.mSecondary))
	{
		// instantiate the linked template
		Transform2 transform(aTemplate.mOffset * entity->GetTransform());
		mSecondary = Database::Instantiate(mSecondary, Database::owner.Get(mId), mId,
			transform.Angle(), transform.p, entity->GetVelocity(), entity->GetOmega(), false);
	}

	// create a backlink
	Database::backlink.Put(mSecondary, mId);

	if (!Database::entity.Get(aTemplate.mSecondary))
	{
		// activate
		Database::Activate(mSecondary);
	}
}

Link::~Link(void)
{
	if (mSecondary)
	{
		// if deleting the secondary
		if (mDeleteSecondary)
		{
			Database::Delete(mSecondary);
		}
	}
}

void Link::Update(float aStep)
{
	if (mSecondary)
	{
		// get entities
		Entity *entity = Database::entity.Get(mId);
		Entity *secondary = Database::entity.Get(mSecondary);
		if (!entity || !secondary)
		{
			Database::Typed<Link *> &links = Database::link.Open(mId);
			links.Delete(mSub);
			Database::link.Close(mId);
			delete this;
			return;
		}

		// update secondary transform
		Transform2 transform(mOffset * entity->GetTransform());
		secondary->Step();
		if (mUpdateAngle)
		{
			secondary->SetAngle(transform.Angle());
			secondary->SetOmega(entity->GetOmega());
		}
		if (mUpdatePosition)
		{
			secondary->SetPosition(transform.p);
			secondary->SetVelocity(entity->GetVelocity());
		}
		if (b2Body *body = Database::collidablebody.Get(mSecondary))
		{
			body->SetAwake(true);
			body->SetTransform(transform.p, transform.a);
			body->SetLinearVelocity(entity->GetVelocity());
			body->SetAngularVelocity(entity->GetOmega());
		}
	}
}
