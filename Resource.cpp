#include "StdAfx.h"
#include "Resource.h"
#include "Entity.h"
#include "Updatable.h"
#include "Link.h"
#include "ExpressionAction.h"


#ifdef USE_POOL_ALLOCATOR
// resource pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Resource));
void *Resource::operator new(size_t aSize)
{
	return pool.malloc();
}
void Resource::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<Typed<ResourceTemplate> > resourcetemplate(0x79aa609b /* "resourcetemplate" */);
	Typed<Typed<Resource *> > resource(0x29df7ff5 /* "resource" */);
	Typed<Typed<Resource::ChangeSignal> > resourcechange(0x80d66669 /* "resourcechange" */);
	Typed<Typed<Resource::EmptySignal> > resourceempty(0xc5325c82 /* "resourceempty" */);
	Typed<Typed<Resource::FullSignal> > resourcefull(0xa4f2734c /* "resourcefull" */);

	namespace Loader
	{
		class ResourceLoader
		{
		public:
			ResourceLoader()
			{
				AddConfigure(0x29df7ff5 /* "resource" */, Entry(this, &ResourceLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Database::Typed<ResourceTemplate> &resources = Database::resourcetemplate.Open(aId);
				unsigned int aSubId = Hash(element->Attribute("name"));
				Database::name.Put(aSubId, element->Attribute("name"));
				ResourceTemplate &resource = resources.Open(aSubId);
				resource.Configure(element, aId, aSubId);
				resources.Close(aSubId);
				Database::resourcetemplate.Close(aId);
			}
		}
		resourceloader;
	}

	namespace Initializer
	{
		class ResourceInitializer
		{
		public:
			ResourceInitializer()
			{
				AddActivate(0x79aa609b /* "resourcetemplate" */, Entry(this, &ResourceInitializer::Activate));
				AddDeactivate(0x79aa609b /* "resourcetemplate" */, Entry(this, &ResourceInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				Typed<Resource *> &resources = Database::resource.Open(aId);
				for (Typed<ResourceTemplate>::Iterator itor(Database::resourcetemplate.Find(aId)); itor.IsValid(); ++itor)
				{
					const ResourceTemplate &resourcetemplate = itor.GetValue();
					Resource *resource = new Resource(resourcetemplate, aId);
					resources.Put(itor.GetKey(), resource);
				}
				resources.Close(aId);
			}

			void Deactivate(unsigned int aId)
			{
				for (Typed<Resource *>::Iterator itor(Database::resource.Find(aId)); itor.IsValid(); ++itor)
				{
					Resource *resource = itor.GetValue();
					delete resource;
				}
				Database::resource.Delete(aId);
				Database::resourcechange.Delete(aId);
				Database::resourceempty.Delete(aId);
				Database::resourcefull.Delete(aId);
			}
		}
		resourceinitializer;
	}
}


ResourceTemplate::ResourceTemplate(void)
: mSubId(0)
, mInitial(0)
, mMaximum(FLT_MAX)
, mDelay(0)
, mCycle(0)
, mAdd(0)
{
}

ResourceTemplate::~ResourceTemplate(void)
{
}

bool ResourceTemplate::Configure(const TiXmlElement *element, unsigned int aId, unsigned int aSubId)
{
	mSubId = aSubId;

	element->QueryFloatAttribute("initial", &mInitial);
	element->QueryFloatAttribute("maximum", &mMaximum);
	element->QueryFloatAttribute("delay", &mDelay);
	element->QueryFloatAttribute("cycle", &mCycle);
	element->QueryFloatAttribute("add", &mAdd);
	if (element->QueryFloatAttribute("rate", &mAdd) == TIXML_SUCCESS)
		mAdd *= mCycle;

	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch(Hash(child->Value()))
		{
		case 0xcba76728 /* "onchange" */:
			break;

		case 0xb9bd5041 /* "onempty" */:
			break;

		case 0x12a7aef5 /* "onfull" */:
			break;

		default:
			break;
		}
	}

	return true;
}


Resource::Resource(void)
: Updatable(0)
, mSubId(0)
, mValue(0)
, mTimer(0)
{
}

Resource::Resource(const ResourceTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mSubId(aTemplate.mSubId)
, mValue(aTemplate.mInitial)
, mTimer(0)
{
	if (aTemplate.mAdd)
	{
		Updatable::SetAction(Updatable::Action(this, &Resource::Update));
		if (aTemplate.mAdd > 0 && aTemplate.mInitial < aTemplate.mMaximum ||
			aTemplate.mAdd < 0 && aTemplate.mInitial > 0)
			Activate();
	}
}

Resource::~Resource(void)
{
}

void Resource::Update(float aStep)
{
	mTimer -= aStep;
	if (mTimer <= 0)
	{
		const ResourceTemplate &resource = Database::resourcetemplate.Get(mId).Get(mSubId);
		Add(mId, resource.mAdd);
		mTimer += resource.mCycle;
	}
}

void Resource::Set(unsigned int aSourceId, float aValue)
{
	const ResourceTemplate &resource = Database::resourcetemplate.Get(mId).Get(mSubId);

	// if empty...
	if (aValue <= 0)
	{
		aValue = 0;

		// notify all empty listeners
		Database::resourceempty.Get(mId).Get(mSubId)(mId, mSubId, aSourceId);

		if (resource.mAdd < 0)
			Deactivate();
	}

	// if full...
	if (aValue >= resource.mMaximum)
	{
		aValue = resource.mMaximum;
		
		// notify all empty listeners
		Database::resourcefull.Get(mId).Get(mSubId)(mId, mSubId, aSourceId);

		if (resource.mAdd > 0)
			Deactivate();
	}

	if (mValue != aValue)
	{
		// if something dropped the value...
		if ((mValue - aValue) * (resource.mAdd) > 0)
		{
			// reset timer
			mTimer = Database::resourcetemplate.Get(mId).Get(mSubId).mDelay;
			Activate();
		}

#ifdef DEBUG_RESOURCE
		DebugPrint("\"%s\" resource=\"%s\" value=%f->%f\n", Database::name.Get(mId).c_str(), Database::name.Get(mSubId).c_str(), mValue, aValue);
#endif

		// update the value
		mValue = aValue;

		// notify all change listeners
		Database::resourcechange.Get(mId).Get(mSubId)(mId, mSubId, aSourceId, aValue);
	}
}

void Resource::Add(unsigned int aSourceId, float aAdd)
{
	// set the value
	Set(aSourceId, mValue + aAdd);
}



unsigned int FindResource(unsigned int aId, unsigned int aSubId)
{
	// skip unnamed resource
	if (!aSubId)
		return 0;

	// for each entry in the backlink chain...
	for (unsigned int id = aId; id; id = Database::backlink.Get(id))
	{
		// if the entry has a matching resource...
		if (Database::resourcetemplate.Get(id).Find(aSubId))
		{
			// return the entry
			return id;
		}
	}

	// get the owner (player)
	unsigned int owner = Database::owner.Get(aId);

	// if the owner has a matching resource...
	if (Database::resourcetemplate.Get(owner).Find(aSubId))
	{
		// return the owner
		return owner;
	}

	// not found
	return 0;
}
