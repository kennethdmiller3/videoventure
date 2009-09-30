#pragma once

#include "Signal.h"

class VarItem
{
public:
	typedef Signal<void (unsigned int)> Signal;

	unsigned int mId;

	enum Type
	{
		// command (no data)
		COMMAND,
		
		// integer value
		INTEGER,
		
		// floating-point value
		FLOAT,
		
		// string value
		STRING,
		
		// scope ("directory")
		SCOPE,
	};
	Type mType;

public:
	// create var items
	static VarItem *CreateCommand(const char *aPath);
	static VarItem *CreateInteger(const char *aPath, int aValue = 0, int aMinimum = INT_MIN, int aMaximum = INT_MAX);
	static VarItem *CreateFloat(const char *aPath, float aValue = 0.0f, float aMinimum = -FLT_MAX, float aMaximum = FLT_MAX);
	static VarItem *CreateString(const char *aPath, std::string aValue = "");
	
	// set var items
	static void SetInteger(const char *aPath, int aValue);
	static void SetFloat(const char *aPath, float aValue);
	static void SetString(const char *aPath, std::string aValue);

	// get var items
	static int GetInteger(const char *aPath);
	static float GetFloat(const char *aPath);
	static std::string GetString(const char *aPath);

public:
	VarItem(unsigned int aId, Type aType)
		: mId(aId), mType(aType)
	{
	}

	void Notify();

	virtual int GetInteger() { return 0; };
	virtual void SetInteger(int aValue) { };
	virtual void SetIntegerRange(int aMinimum, int aMaximum) { };

	virtual float GetFloat() { return 0.0f; };
	virtual void SetFloat(float aValue) { };
	virtual void SetFloatRange(float aMinimum, float aMaximum) { };

	virtual std::string GetString() { return ""; };
	virtual void SetString(std::string aValue) { };
};

class VarCommand : public VarItem
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	VarCommand(unsigned int mId)
		: VarItem(mId, COMMAND)
	{
	}
};

class VarInteger : public VarItem
{
public:
	int value;
	int minimum;
	int maximum;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	VarInteger(unsigned int aId, int aValue = 0, int aMinimum = INT_MIN, int aMaximum = INT_MAX)
		: VarItem(mId, INTEGER), value(aValue), minimum(aMinimum), maximum(aMaximum)
	{
	}

	virtual int GetInteger();
	virtual void SetInteger(int aValue);
	virtual void SetIntegerRange(int aMinimum, int aMaximum);

	virtual float GetFloat();
	virtual void SetFloat(float aValue);
	virtual void SetFloatRange(float aMinimum, float aMaximum);

	virtual std::string GetString();
	virtual void SetString(std::string aValue);
};

class VarFloat : public VarItem
{
public:
	float value;
	float minimum;
	float maximum;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	VarFloat(unsigned int aId, float aValue = 0.0f, float aMinimum = -FLT_MAX, float aMaximum = FLT_MAX)
		: VarItem(mId, FLOAT), value(aValue), minimum(aMinimum), maximum(aMaximum)
	{
	}

	virtual int GetInteger();
	virtual void SetInteger(int aValue);
	virtual void SetIntegerRange(int aMinimum, int aMaximum);

	virtual float GetFloat();
	virtual void SetFloat(float aValue);
	virtual void SetFloatRange(float aMinimum, float aMaximum);

	virtual std::string GetString();
	virtual void SetString(std::string aValue);
};

class VarString : public VarItem
{
public:
	std::string value;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	VarString(unsigned int aId, std::string aValue = "")
		: VarItem(mId, STRING), value(aValue)
	{
	}

	virtual int GetInteger();
	virtual void SetInteger(int aValue);
	virtual void SetIntegerRange(int aMinimum, int aMaximum);

	virtual float GetFloat();
	virtual void SetFloat(float aValue);
	virtual void SetFloatRange(float aMinimum, float aMaximum);

	virtual std::string GetString();
	virtual void SetString(std::string aValue);
};

class VarScope : public VarItem
{
public:
	std::vector<unsigned int> children;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	VarScope(unsigned int mId)
		: VarItem(mId, SCOPE)
	{
	}

	virtual int GetInteger();
	virtual void SetInteger(int aValue);
	virtual void SetIntegerRange(int aMinimum, int aMaximum);

	virtual float GetFloat();
	virtual void SetFloat(float aValue);
	virtual void SetFloatRange(float aMinimum, float aMaximum);

	virtual std::string GetString();
	virtual void SetString(std::string aValue);
};

namespace Database
{
	extern Typed<VarItem *> varitem;
	extern Typed<VarItem::Signal> varitemnotify;
}
