// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "vector.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>

#include "string.h"

namespace srb2
{

AbstractVector::GrowResult AbstractVector::_realloc_mem(void* data, size_t size, size_t old_cap, size_t elem_size, Move move, size_t cap) noexcept
{
	GrowResult ret;
	std::allocator<uint8_t> allocator;

	if (old_cap == 0)
	{
		cap = std::max<size_t>(cap, 1);
		ret.data = allocator.allocate(cap * elem_size + sizeof(std::max_align_t));
		ret.cap = cap;
		return ret;
	}

	cap = std::max<size_t>(cap, old_cap * 2);
	ret.data = allocator.allocate(cap * elem_size + sizeof(std::max_align_t));
	(move)(ret.data, data, size);
	allocator.deallocate(reinterpret_cast<uint8_t*>(data), old_cap * elem_size + sizeof(std::max_align_t));
	ret.cap = cap;
	return ret;
}

void AbstractVector::_free_mem(void* data, size_t cap, size_t elem_size) noexcept
{
	std::allocator<uint8_t> allocator;
	allocator.deallocate(reinterpret_cast<uint8_t*>(data), cap * elem_size + sizeof(std::max_align_t));
}

template class Vector<bool>;
template class Vector<std::byte>;
template class Vector<uint8_t>;
template class Vector<uint16_t>;
template class Vector<uint32_t>;
template class Vector<uint64_t>;
template class Vector<int8_t>;
template class Vector<int16_t>;
template class Vector<int32_t>;
template class Vector<int64_t>;
template class Vector<String>;

} // namespace srb2
