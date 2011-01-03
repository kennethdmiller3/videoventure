#include "stdafx.h"

#include "MemoryPool.h"

// alignment of memory pool data
#define MEMORY_POOL_ALIGN 16

//#define MEMORY_POOL_VIRTUAL_ALLOC


// MEMORY POOL

// default constructor
MemoryPool::MemoryPool(void)
: mChunk(NULL), mFree(NULL)
{
	Setup(0, 0, 0);
}

// constructor
MemoryPool::MemoryPool(size_t aSize, size_t aStart, size_t aGrow)
: mChunk(NULL), mFree(NULL)
{
	Setup(aSize, aStart, aGrow);
	Init();
}

// destructor
MemoryPool::~MemoryPool()
{
	Clean();
}

// setup
void MemoryPool::Setup(size_t aSize, size_t aStart, size_t aGrow)
{
	// invalidate the pool
	Clean();

	// update properties
	// (and ensure a block is large enough to hold a freelist node)
	mSize = std::max(sizeof(void *), aSize);
	mStart = aStart;
	mGrow = aGrow;
}

// initialize
void MemoryPool::Init(void)
{
	Clean();

	// add an initial chunk
	if (mStart > 0)
	{
		Grow(mStart);
	}
}

// cleanup
void MemoryPool::Clean(void)
{
	// free all chunks
	while (mChunk)
	{
		void *next = *reinterpret_cast<void **>(mChunk);
#ifdef MEMORY_POOL_VIRTUAL_ALLOC
		VirtualFree(mChunk, 0, MEM_RELEASE);
#else
		free(mChunk);
#endif
		mChunk = next;
	}

	// clear
	mChunk = NULL;
	mFree = NULL;
}

// grow the memory pool
void MemoryPool::Grow(size_t aCount)
{
	// create a new chunk
#ifdef MEMORY_POOL_VIRTUAL_ALLOC
	void *chunk = VirtualAlloc(NULL, MEMORY_POOL_ALIGN + mSize * aCount, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	void *chunk = malloc(MEMORY_POOL_ALIGN + mSize * aCount);
#endif

	// add to the head of the chunk list
	*reinterpret_cast<void **>(chunk) = mChunk;
	mChunk = chunk;

	// fill with an unallocated pattern
	memset(reinterpret_cast<unsigned char *>(chunk) + sizeof(void *), 0xFF, mSize * aCount + MEMORY_POOL_ALIGN - sizeof(void *));

	// build free list
	void *first = reinterpret_cast<unsigned char *>(chunk) + MEMORY_POOL_ALIGN;
	void *free = first;
	for (size_t i = 0; i < aCount - 1; ++i)
	{
		free = *reinterpret_cast<unsigned char **>(free) = reinterpret_cast<unsigned char *>(free) + mSize;
	}

	// add to the head of the free list
	*reinterpret_cast<void **>(free) = mFree;
	mFree = first;
}

// allocate a block
void *MemoryPool::Alloc(void)
{
	// if no free blocks left...
	if (!mFree)
	{
		// if allowed to grow...
		if (mGrow)
		{
			// grow the memory pool
			Grow(mGrow);
		}
		else
		{
			// no free blocks left
			return NULL;
		}
	}

	// get first free entry
	void *ptr = mFree;

	// remove from the head of the free list
	mFree = *reinterpret_cast<void **>(ptr);

#ifdef _DEBUG
	// fill with an allocated pattern
	memset(ptr, 0xCD, mSize);
#endif

	// return the entry
	return ptr;
}

// free a block
void MemoryPool::Free(void *aPtr)
{
#ifdef _DEBUG
	// make sure it came from this pool
	_ASSERT(IsValid(aPtr));

	// fill with a freed pattern
	memset(aPtr, 0xDD, mSize);
#endif

	// add to the head of the free list
	*reinterpret_cast<void **>(aPtr) = mFree;
	mFree = aPtr;
}

// is the pointer valid?
bool MemoryPool::IsValid(const void *aPtr)
{
	// validate the pointer
	for (void *chunk = mChunk; chunk != NULL; chunk = *reinterpret_cast<void **>(chunk))
	{
#ifdef MEMORY_POOL_VIRTUAL_ALLOC
		MEMORY_BASIC_INFORMATION info;
		VirtualQuery(chunk, &info, sizeof(info));
		size_t size = info.RegionSize;
#else
		size_t size = _msize(chunk);
#endif
		if (reinterpret_cast<const unsigned char *>(aPtr) >= reinterpret_cast<const unsigned char *>(chunk) + MEMORY_POOL_ALIGN &&
			reinterpret_cast<const unsigned char *>(aPtr) < reinterpret_cast<const unsigned char *>(chunk) + size)
		{
			return true;
		}
	}
	return false;
}
