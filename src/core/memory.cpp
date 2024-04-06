// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "memory.h"

#include <array>
#include <new>

#include "../z_zone.h"

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

static LinearMemory g_frame_memory {4 * 1024 * 1024};

void* Z_Frame_Alloc(size_t size)
{
	return g_frame_memory.allocate(size);
}

void Z_Frame_Reset()
{
	g_frame_memory.reset();
}
