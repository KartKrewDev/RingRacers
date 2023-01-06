// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "streams.hpp"

template class srb2::io::ZlibInputStream<srb2::io::SpanStream>;
template class srb2::io::ZlibInputStream<srb2::io::VecStream>;
