#pragma once

// MEMORY POOL ALLOCATOR
class MemoryPool
{
private:
	// size of blocks in the pool
	size_t mSize;

	// number of blocks to add when out of blocks
	size_t mGrow;

	// pointer to list of allocated chunks
	void *mChunk;

	// pointer to list of free blocks
	void *mFree;

private:
	// grow the memory pool
	void Grow(size_t aCount);

public:
	// consructor
	GAME_API MemoryPool(size_t aSize, size_t aCount = 256, size_t aGrow = 16);

	// destructor
	GAME_API ~MemoryPool();

	// allocate a block
	GAME_API void * Alloc(void);

	// free a block
	GAME_API void Free(void * aPtr);
};
