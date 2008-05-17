#include "StdAfx.h"
#include "Entity.h"
#include <algorithm>

#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// entity pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Entity));
void *Entity::operator new(size_t aSize)
{
	return pool.malloc();
}
void Entity::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<Entity *> entity(0xd33ff5da /* "entity" */);

	namespace Loader
	{
		static void ProcessTemplateItem(const TiXmlElement *element, unsigned int aId)
		{
			const char *value = element->Value();
			const Database::Loader::Entry &configure = Database::Loader::GetConfigure(Hash(value));
			if (configure)
				configure(aId, element);
			else
				DebugPrint("Unrecognized tag \"%s\"\n", value);
		}

		class TemplateLoader
		{
		public:
			TemplateLoader()
			{
				AddConfigure(0x694aaa0b /* "template" */, Entry(this, &TemplateLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// get parent identifier
				const char *type = element->Attribute("type");
				unsigned int aParentId = Hash(type);

				// inherit parent components
				Database::Inherit(aId, aParentId);

				// set name
				std::string &namebuf = Database::name.Open(aId);
				namebuf = element->Attribute("name");
				Database::name.Close(aId);

				// for each child element...
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					// process the template item
					ProcessTemplateItem(child, aId);
				}
			}
		}
		templateloader;

		class EntityLoader
		{
		public:
			EntityLoader()
			{
				AddConfigure(0xd33ff5da /* "entity" */, Entry(this, &EntityLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// get parent identifier
				const char *type = element->Attribute("type");
				unsigned int aParentId = Hash(type);

				// set name
				std::string &namebuf = Database::name.Open(aId);
				namebuf = element->Attribute("name");
				Database::name.Close(aId);
				
				// set parent
				Database::parent.Put(aId, aParentId);

				// objects default to owning themselves
				Database::owner.Put(aId, aId);

				// create an entity
				Entity *entity = new Entity(aId);
				Database::entity.Put(aId, entity);

				// process child elements
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					if (entity->Configure(child))
						continue;

					// process the template item
					ProcessTemplateItem(child, aId);
				}

				// activate the instance
				// (create runtime components)
				Database::Activate(aId);
			}
		}
		entityloader;
	}
}


unsigned int Entity::sNextId = 1;

Entity::Entity(unsigned int id)
: id(id)
, angle_0(0), posit_0(0, 0)
, angle_1(0), posit_1(0, 0)
, veloc(0, 0)
, omega(0)
{
}

Entity::~Entity(void)
{
}

// configure
bool Entity::Configure(const TiXmlElement *element)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &posit_1.x);
		element->QueryFloatAttribute("y", &posit_1.y);
		if (element->QueryFloatAttribute("angle", &angle_1) == TIXML_SUCCESS)
			angle_1 *= float(M_PI) / 180.0f;
		return true;

	case 0x32741c32 /* "velocity" */:
		element->QueryFloatAttribute("x", &veloc.x);
		element->QueryFloatAttribute("y", &veloc.y);
		if (element->QueryFloatAttribute("angle", &omega) == TIXML_SUCCESS)
			omega *= float(M_PI) / 180.0f;
		return true;

	case 0xf5674cd4 /* "owner" */:
		Database::owner.Put(id, Hash(element->Attribute("name")));
		return true;

	default:
		return false;
	}
}
