#include "StdAfx.h"
#include "VarItem.h"

#ifdef USE_POOL_ALLOCATOR
// command pool
static boost::pool<boost::default_user_allocator_malloc_free> varcommandpool(sizeof(VarCommand));
void *VarCommand::operator new(size_t aSize)
{
	return varcommandpool.malloc();
}
void VarCommand::operator delete(void *aPtr)
{
	varcommandpool.free(aPtr);
}

// integer pool
static boost::pool<boost::default_user_allocator_malloc_free> varintegerpool(sizeof(VarInteger));
void *VarInteger::operator new(size_t aSize)
{
	return varintegerpool.malloc();
}
void VarInteger::operator delete(void *aPtr)
{
	varintegerpool.free(aPtr);
}

// float pool
static boost::pool<boost::default_user_allocator_malloc_free> varfloatpool(sizeof(VarFloat));
void *VarFloat::operator new(size_t aSize)
{
	return varfloatpool.malloc();
}
void VarFloat::operator delete(void *aPtr)
{
	varfloatpool.free(aPtr);
}

// string pool
static boost::pool<boost::default_user_allocator_malloc_free> varstringpool(sizeof(VarString));
void *VarString::operator new(size_t aSize)
{
	return varstringpool.malloc();
}
void VarString::operator delete(void *aPtr)
{
	varstringpool.free(aPtr);
}

// scope pool
static boost::pool<boost::default_user_allocator_malloc_free> varscopepool(sizeof(VarScope));
void *VarScope::operator new(size_t aSize)
{
	return varscopepool.malloc();
}
void VarScope::operator delete(void *aPtr)
{
	varscopepool.free(aPtr);
}
#endif

namespace Database
{
	Typed<VarItem *> varitem(0xa8f248fd /* "varitem" */);
	Typed<VarItem::Signal> varitemnotify(0xbab83e54 /* "varitemnotify" */);
}


//
// VARIABLE ITEM

static unsigned int BuildPath(const char *aPath)
{
	// set start to path
	const char *start = aPath;

	// get root scope
	VarScope *scope = static_cast<VarScope *>(Database::varitem.Get(0));
	if (!scope)
	{
		scope = new VarScope(0);
		Database::varitem.Put(0, scope);
	}

	// while the path still has separators...
	while (const char *split = strchr(start, '.'))
	{
		// get the scope name
		std::string name(aPath, split - aPath);
		unsigned int id = Hash(name.c_str());

		// if a scope already exists...
		if (VarItem *item = Database::varitem.Get(id))
		{
			// get the scope
			assert(item->mType == VarItem::SCOPE);
			scope = static_cast<VarScope *>(item);
		}
		else
		{
			// create a new scope
			scope->children.push_back(id);
			Database::name.Put(id, name);
			Database::parent.Put(id, scope->mId);
			scope = new VarScope(id);
			Database::varitem.Put(id, scope);
		}
		start = split+1;
	}

	// set up a new item
	unsigned int id = Hash(aPath);
	scope->children.push_back(id);
	Database::name.Put(id, aPath);
	Database::parent.Put(id, scope->mId);

	return id;
}

VarItem *VarItem::CreateCommand(const char *aPath)
{
	unsigned int id = BuildPath(aPath);

	VarCommand *item = new VarCommand(id);
	Database::varitem.Put(id, item);

	return item;
}

VarItem *VarItem::CreateInteger(const char *aPath, int aValue, int aMinimum, int aMaximum)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		return item;

	unsigned int id = BuildPath(aPath);

	VarInteger *item = new VarInteger(id, aValue, aMinimum, aMaximum);
	Database::varitem.Put(id, item);

	return item;
}

VarItem *VarItem::CreateFloat(const char *aPath, float aValue, float aMinimum, float aMaximum)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		return item;

	unsigned int id = BuildPath(aPath);

	VarFloat *item = new VarFloat(id, aValue, aMinimum, aMaximum);
	Database::varitem.Put(id, item);

	return item;
}

VarItem *VarItem::CreateString(const char *aPath, std::string aValue)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		return item;

	unsigned int id = BuildPath(aPath);

	VarString *item = new VarString(id, aValue);
	Database::varitem.Put(id, item);

	return item;
}

void VarItem::SetInteger(const char *aPath, int aValue)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		item->SetInteger(aValue);
}

void VarItem::SetFloat(const char *aPath, float aValue)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		item->SetFloat(aValue);
}

void VarItem::SetString(const char *aPath, std::string aValue)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		item->SetString(aValue);
}

int VarItem::GetInteger(const char *aPath)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		return item->GetInteger();
	else
		return 0;
}

float VarItem::GetFloat(const char *aPath)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		return item->GetFloat();
	else
		return 0.0f;
}

std::string VarItem::GetString(const char *aPath)
{
	if (VarItem *item = Database::varitem.Get(Hash(aPath)))
		return item->GetString();
	else
		return "";
}

void VarItem::Notify()
{
	Database::varitemnotify.Get(mId)(mId);
}


//
// VARIABLE INTEGER

int VarInteger::GetInteger()
{
	return value;
}
void VarInteger::SetInteger(int aValue)
{
	value = aValue;
	value = Clamp(value, minimum, maximum);
	Notify();
}
void VarInteger::SetIntegerRange(int aMinimum, int aMaximum)
{
	minimum = aMinimum;
	maximum = aMaximum;
}

float VarInteger::GetFloat()
{
	return (float)value;
}
void VarInteger::SetFloat(float aValue)
{
	value = (int)aValue;
	value = Clamp(value, minimum, maximum);
	Notify();
}
void VarInteger::SetFloatRange(float aMinimum, float aMaximum)
{
	minimum = (int)aMinimum;
	maximum = (int)aMaximum;
}

std::string VarInteger::GetString()
{
	char buf[16];
	TIXML_SNPRINTF( buf, sizeof(buf), "%d", value );
	return std::string(buf);
}
void VarInteger::SetString(std::string aValue)
{
	TIXML_SSCANF( aValue.c_str(), "%d", &value );
	value = Clamp(value, minimum, maximum);
	Notify();
}


//
// VARIABLE FLOAT

int VarFloat::GetInteger()
{
	return (int)value;
}
void VarFloat::SetInteger(int aValue)
{
	value = (float)aValue;
	value = Clamp(value, minimum, maximum);
	Notify();
}
void VarFloat::SetIntegerRange(int aMinimum, int aMaximum)
{
	minimum = (float)aMinimum;
	maximum = (float)aMaximum;
}

float VarFloat::GetFloat()
{
	return value;
}
void VarFloat::SetFloat(float aValue)
{
	value = aValue;
	value = Clamp(value, minimum, maximum);
	Notify();
}
void VarFloat::SetFloatRange(float aMinimum, float aMaximum)
{
	minimum = aMinimum;
	maximum = aMaximum;
}

std::string VarFloat::GetString()
{
	char buf[16];
	TIXML_SNPRINTF( buf, sizeof(buf), "%f", value );
	return std::string(buf);
}
void VarFloat::SetString(std::string aValue)
{
	TIXML_SSCANF( aValue.c_str(), "%f", &value );
	value = Clamp(value, minimum, maximum);
	Notify();
}


//
// VARIABLE STRING

int VarString::GetInteger()
{
	int rValue;
	TIXML_SSCANF( value.c_str(), "%d", &rValue );
	return rValue;
}
void VarString::SetInteger(int aValue)
{
	char buf[16];
	TIXML_SNPRINTF( buf, sizeof(buf), "%d", aValue );
	value = buf;
	Notify();
}
void VarString::SetIntegerRange(int aMinimum, int aMaximum)
{
}

float VarString::GetFloat()
{
	float rValue;
	TIXML_SSCANF( value.c_str(), "%f", &rValue );
	return rValue;
}
void VarString::SetFloat(float aValue)
{
	char buf[16];
	TIXML_SNPRINTF( buf, sizeof(buf), "%f", aValue );
	value = buf;
	Notify();
}
void VarString::SetFloatRange(float aMinimum, float aMaximum)
{
}

std::string VarString::GetString()
{
	return value;
}
void VarString::SetString(std::string aValue)
{
	value = aValue;
	Notify();
}


//
// VARIABLE SCOPE

int VarScope::GetInteger()
{
	return (int)children.size();
}
void VarScope::SetInteger(int aValue)
{
}
void VarScope::SetIntegerRange(int aMinimum, int aMaximum)
{
}

float VarScope::GetFloat()
{
	return (float)children.size();
}
void VarScope::SetFloat(float aValue)
{
}
void VarScope::SetFloatRange(float aMinimum, float aMaximum)
{
}

std::string VarScope::GetString()
{
	std::string value;
	for (unsigned int i = 0; i < children.size(); ++i)
	{
		if (i > 0)
			value += "\n";
		value += Database::name.Get(children[i]);
	}
	return value;
}
void VarScope::SetString(std::string aValue)
{
}
