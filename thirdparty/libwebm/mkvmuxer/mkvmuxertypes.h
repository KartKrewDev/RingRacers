// Copyright (c) 2012 The WebM project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#ifndef MKVMUXER_MKVMUXERTYPES_H_
#define MKVMUXER_MKVMUXERTYPES_H_
#include <stdint.h>

namespace mkvmuxer {
typedef unsigned char uint8;
typedef short int16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
}  // namespace mkvmuxer

// Copied from Chromium basictypes.h
// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define LIBWEBM_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);                       \
  void operator=(const TypeName&)

#endif  // MKVMUXER_MKVMUXERTYPES_HPP_
