// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "hash_set.hpp"

#include <string>

#include "string.h"

namespace srb2
{

template class HashSet<bool>;
template class HashSet<uint8_t>;
template class HashSet<uint16_t>;
template class HashSet<uint32_t>;
template class HashSet<uint64_t>;
template class HashSet<int8_t>;
template class HashSet<int16_t>;
template class HashSet<int32_t>;
template class HashSet<int64_t>;
template class HashSet<String>;
template class HashSet<std::string>;

} // namespace srb2
