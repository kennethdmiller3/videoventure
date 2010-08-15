#include "stdafx.h"

#include "MemoryPool.h"

// alignment of memory pool data
#define MEMORY_POOL_ALIGN 16


// MEMORY POOL

// constructor
MemoryPool::MemoryPool(size_t aSize, size_t aCount, size_t aGrow)
: mSize(aSize), mGrow(aGrow), mChunk(NULL), mFree(NULL)
{
	if (aCount > 0)
	{
		Grow(aCount);
	}
}

// destructor
MemoryPool::~MemoryPool()
{
	// free all chunks
	while (mChunk)
	{
		void *next = *reinterpret_cast<void **>(mChunk);
		free(mChunk);
		mChunk = next;
	}
}

// grow the memory pool
void MemoryPool::Grow(size_t aCount)
{
	// create a new chunk
	void *chunk = malloc(MEMORY_POOL_ALIGN + mSize * aCount);

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
	// fill with an freed pattern
	memset(aPtr, 0xDD, mSize);
#endif

	// add to the head of the free list
	*reinterpret_cast<void **>(aPtr) = mFree;
	mFree = aPtr;
}
