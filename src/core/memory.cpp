// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "memory.h"

#include <cstdint>

#include "../cxxutil.hpp"
#include "../z_zone.h"
#include "../lua_script.h"

using namespace srb2;

namespace
{

class LinearMemory
{
	size_t size_;
	size_t height_;
	void* memory_;

public:
	constexpr explicit LinearMemory(size_t size) noexcept;

	void* allocate(size_t size);
	void reset() noexcept;
};

constexpr LinearMemory::LinearMemory(size_t size) noexcept : size_(size), height_{0}, memory_{nullptr} {}

void* LinearMemory::allocate(size_t size)
{
	size_t aligned_size = (size + 15) & ~15;
	if (height_ + aligned_size > size_)
	{
		throw std::bad_alloc();
	}

	if (memory_ == nullptr)
	{
		memory_ = Z_Malloc(size_, PU_STATIC, nullptr);
	}

	void* ptr = (void*)((uintptr_t)(memory_) + height_);
	height_ += aligned_size;
	return ptr;
}

void LinearMemory::reset() noexcept
{
	height_ = 0;
}

} // namespace

PoolAllocator::~PoolAllocator()
{
	release();
}

constexpr static size_t nearest_multiple(size_t v, size_t divisor)
{
	return (v + (divisor - 1)) & ~(divisor - 1);
}

PoolAllocator::ChunkFooter* PoolAllocator::allocate_chunk()
{
	uint8_t* chunk = (uint8_t*)Z_Malloc(nearest_multiple(blocks_ * block_size_, alignof(ChunkFooter)) + sizeof(ChunkFooter), tag_, nullptr);
	ChunkFooter* footer = (ChunkFooter*)(chunk + (blocks_ * block_size_));
	footer->next = nullptr;
	footer->start = (void*)chunk;
	for (size_t i = 0; i < blocks_; i++)
	{
		FreeBlock* cur = (FreeBlock*)(chunk + (i * block_size_));
		FreeBlock* next = (FreeBlock*)(chunk + ((i + 1) * block_size_));
		cur->next = next;
	}
	((FreeBlock*)(chunk + ((blocks_ - 1) * block_size_)))->next = nullptr;
	return footer;
}

void* PoolAllocator::allocate()
{
	if (first_chunk_ == nullptr)
	{
		SRB2_ASSERT(head_ == nullptr);

		// No chunks allocated yet
		first_chunk_ = allocate_chunk();
		head_ = (FreeBlock*)first_chunk_->start;
	}

	if (head_->next == nullptr)
	{
		// Current chunk will be full; allocate another at the end of the list
		ChunkFooter* last_chunk = first_chunk_;
		while (last_chunk->next != nullptr)
		{
			last_chunk = last_chunk->next;
		}
		ChunkFooter* new_chunk = allocate_chunk();
		last_chunk->next = new_chunk;
		head_->next = (FreeBlock*)new_chunk->start;
	}

	FreeBlock* ret = head_;
	head_ = head_->next;
	return ret;
}

void PoolAllocator::deallocate(void* p)
{
	// Required in case this block is reused
	LUA_InvalidateUserdata(p);

	FreeBlock* block = reinterpret_cast<FreeBlock*>(p);
	block->next = head_;
	head_ = block;
}

void PoolAllocator::release()
{
	ChunkFooter* next = nullptr;
	for (ChunkFooter* i = first_chunk_; i != nullptr; i = next)
	{
		uint8_t *chunk = (uint8_t*)i->start;
		for (size_t j = 0; j < blocks_; j++)
		{
			// Invalidate all blocks that possibly weren't passed to deallocate
			LUA_InvalidateUserdata(chunk + (j * block_size_));
		}
		next = i->next;
		Z_Free(i->start);
	}

	first_chunk_ = nullptr;
	head_ = nullptr;
}

static LinearMemory g_frame_memory {4 * 1024 * 1024};

void* Z_Frame_Alloc(size_t size)
{
	return g_frame_memory.allocate(size);
}

void Z_Frame_Reset()
{
	g_frame_memory.reset();
}
