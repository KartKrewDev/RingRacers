// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_CORE_MEMORY_H__
#define __SRB2_CORE_MEMORY_H__

#include <stddef.h>

#ifdef __cplusplus

#include <cstdint>

namespace srb2
{

/// Pool allocator; manages bulk allocations of same-size block. If the pool is full,
/// allocations will fail by returning nullptr.
class PoolAllocator final
{
	struct FreeBlock
	{
		FreeBlock* next;
	};
	struct ChunkFooter
	{
		ChunkFooter* next;
		void* start;
	};
	ChunkFooter* first_chunk_;
	FreeBlock* head_;
	size_t block_size_;
	size_t blocks_;
	int32_t tag_;

	ChunkFooter* allocate_chunk();

public:
	constexpr PoolAllocator(size_t block_size, size_t blocks, int32_t tag)
		: first_chunk_(nullptr)
		, head_(nullptr)
		, block_size_(block_size)
		, blocks_(blocks)
		, tag_(tag)
	{}
	PoolAllocator(const PoolAllocator&) = delete;
	PoolAllocator(PoolAllocator&&) noexcept = default;
	~PoolAllocator();

	PoolAllocator& operator=(const PoolAllocator&) = delete;
	PoolAllocator& operator=(PoolAllocator&&) noexcept = default;

	void* allocate();
	void deallocate(void* p);
	constexpr size_t block_size() const noexcept { return block_size_; };

	void release();
};

} // namespace srb2


extern "C" {
#endif // __cpluspplus

/// @brief Allocate a block of memory with a lifespan of the current main-thread frame.
/// This function is NOT thread-safe, but the allocated memory may be used across threads.
/// @return a pointer to a block of memory aligned with libc malloc alignment, or null if allocation fails
void* Z_Frame_Alloc(size_t size);

/// @brief Resets per-frame memory. Not thread safe.
void Z_Frame_Reset(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __SRB2_CORE_MEMORY_H__
