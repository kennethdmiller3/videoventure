#include "stdafx.h"

#include "MemoryPool.h"

//#define MEMORY_POOL_VIRTUAL_ALLOC


// MEMORY POOL

// default constructor
MemoryPool::MemoryPool(void)
: mChunk(NULL), mFree(NULL)
{
	Setup(0, 0, 0, 0);
}

// constructor
MemoryPool::MemoryPool(size_t aSize, size_t aStart, size_t aGrow, size_t aAlign)
: mChunk(NULL), mFree(NULL)
{
	Setup(aSize, aStart, aGrow, aAlign);
	Init();
}

// destructor
MemoryPool::~MemoryPool()
{
	Clean();
}

// setup
void MemoryPool::Setup(size_t aSize, size_t aStart, size_t aGrow, size_t aAlign)
{
	// invalidate the pool
	Clean();

	// update properties
	// (and ensure a block is large enough to hold a freelist node)
	mSize = std::max(sizeof(void *), (aSize + aAlign - 1) & ~(aAlign - 1));
	mStart = aStart;
	mGrow = aGrow;
	mAlign = aAlign;
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
	// allocation size
	size_t size = std::max(sizeof(void *), mAlign) + mSize * aCount;

	// create a new chunk
#ifdef MEMORY_POOL_VIRTUAL_ALLOC
	void *chunk = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	void *chunk = malloc(size);
#endif

	// fill with an unallocated pattern
	memset(chunk, ~0UL, size);

	// add to the head of the chunk list
	*reinterpret_cast<void **>(chunk) = mChunk;
	mChunk = chunk;

	// first element
	// (reserve space for chunk pointer and then align to the requested alignment)
	void *first = reinterpret_cast<void *>((uintptr_t(chunk) + sizeof(void *) + (mAlign - 1)) & ~(mAlign - 1));

	// build free list
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
		// if the pointer is in this chunk...
		const void *end = reinterpret_cast<const unsigned char *>(chunk) + size;
		if (aPtr >= chunk && aPtr < end)
		{
			// first element
			void *first = reinterpret_cast<void *>((uintptr_t(chunk) + sizeof(void *) + (mAlign - 1)) & ~(mAlign - 1));

			// check that the pointer matches a slot in this chunk
			int slot = (uintptr_t(aPtr) - uintptr_t(first)) / mSize;
			if (slot < 0)
				return false;
			int count = (uintptr_t(end) - uintptr_t(first)) / mSize;
			if (slot >= count)
				return false;
			if (uintptr_t(aPtr) != uintptr_t(first) + slot * mSize)
				return false;
			return true;
		}
	}
	return false;
}
