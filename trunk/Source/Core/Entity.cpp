#include "StdAfx.h"
#include "Entity.h"
#include "MemoryPool.h"

#ifdef USE_POOL_ALLOCATOR
// entity pool
static MemoryPool sPool(sizeof(Entity), 256, 16);
void *Entity::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Entity::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<Entity *> entity(0xd33ff5da /* "entity" */);

	namespace Loader
	{
		static bool ConfigureTemplateItem(const tinyxml2::XMLElement *element, unsigned int aId)
		{
			const char *value = element->Value();
			const Database::Loader::Entry &configure = Database::Loader::Configure::Get(Hash(value));
			if (configure)
			{
				configure(aId, element);
				return true;
			}
			return false;
		}

		static void InheritConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// get base type name
			if (const char *type = element->Attribute("type"))
			{
				// get import identifier
				unsigned int aImportId = Hash(type);
				if (!Database::name.Find(aImportId))
					DebugPrint("warning: \"%s\" base type \"%s\" not found\n", Database::name.Get(aId), type);

				// inherit components
				Database::Inherit(aId, aImportId);
			}
		}
		Configure inheritconfigure(0xca04efe0 /* "inherit" */, InheritConfigure);

		void TemplateConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// get base type name
			if (const char *type = element->Attribute("type"))
			{
				// get parent identifier
				unsigned int aParentId = Hash(type);
				if (!Database::name.Find(aParentId))
					DebugPrint("warning: template \"%s\" base type \"%s\" not found\n", element->Attribute("name"), type);

				// inherit parent components
				Database::Inherit(aId, aParentId);
			}

			// get instance name
			if (const char *name = element->Attribute("name"))
			{
				// set name
				std::string &namebuf = Database::name.Open(aId);
				namebuf = element->Attribute("name");
				Database::name.Close(aId);
			}

			// for each child element...
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				// process the template item
				if (!ConfigureTemplateItem(child, aId))
					DebugPrint("template \"%s\" skipping item \"%s\"\n", element->Attribute("name"), child->Value());
			}
		}
		Configure templateconfigure(0x694aaa0b /* "template" */, TemplateConfigure);

		static void EntityConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// get base type name
			if (const char *type = element->Attribute("type"))
			{
				// get parent identifier
				unsigned int aParentId = Hash(type);
				if (!Database::name.Find(aParentId))
					DebugPrint("warning: entity \"%s\" base type \"%s\" not found\n", element->Attribute("name"), type);
					
				// set parent
				Database::parent.Put(aId, aParentId);
			}

			// get instance name
			if (const char *name = element->Attribute("name"))
			{
				// set name
				std::string &namebuf = Database::name.Open(aId);
				namebuf = name;
				Database::name.Close(aId);
			}

			// objects default to owning themselves
			Database::owner.Put(aId, aId);

			// create an entity
			Entity *entity = new Entity(aId);
			Database::entity.Put(aId, entity);

			// process child elements
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				if (entity->Configure(child))
					continue;

				// process the template item
				if (!ConfigureTemplateItem(child, aId))
					DebugPrint("entity \"%s\" skipping item \"%s\"\n", element->Attribute("name"), child->Value());
			}

			// activate the instance
			// (create runtime components)
			Database::Activate(aId);
		}
		Configure entityconfigure (0xd33ff5da /* "entity" */, EntityConfigure);
	}
}


unsigned int Entity::sNextId = 1;

Entity::Entity(unsigned int id)
: id(id)
{
	memset(&trans, 0, sizeof(trans));
	memset(&veloc, 0, sizeof(veloc));
}

Entity::~Entity(void)
{
}

// configure
bool Entity::Configure(const tinyxml2::XMLElement *element)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x934f4e0a /* "position" */:
		element->QueryFloatAttribute("x", &trans[1].p.x);
		element->QueryFloatAttribute("y", &trans[1].p.y);
		if (element->QueryFloatAttribute("angle", &trans[1].a) == tinyxml2::XML_SUCCESS)
			trans[1].a *= float(M_PI) / 180.0f;
		trans[0] = trans[1];
		return true;

	case 0x32741c32 /* "velocity" */:
		element->QueryFloatAttribute("x", &veloc.p.x);
		element->QueryFloatAttribute("y", &veloc.p.y);
		if (element->QueryFloatAttribute("angle", &veloc.a) == tinyxml2::XML_SUCCESS)
			veloc.a *= float(M_PI) / 180.0f;
		return true;

	case 0xf5674cd4 /* "owner" */:
		Database::owner.Put(id, Hash(element->Attribute("name")));
		return true;

	default:
		return false;
	}
}
