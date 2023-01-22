#ifndef __SRB2_IMCONFIG_H__
#define __SRB2_IMCONFIG_H__

#include <stdint.h>

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_DISABLE_OBSOLETE_KEYIO

// We provide needed functionalities provided by default win32 impls through the interface layer
#define IMGUI_DISABLE_WIN32_FUNCTIONS

// RHI Handles are essentially 64-bit integers
#define ImTextureID uint64_t

// RHI does not support integer vectors
#define IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT \
struct ImVec3 \
{ \
    float x, y, z; \
    constexpr ImVec3()                                : x(0.0f), y(0.0f), z(0.0f) { } \
	constexpr ImVec3(float _x, float _y)              : x(_x), y(_y), z(0.0f) { } \
    constexpr ImVec3(float _x, float _y, float _z)    : x(_x), y(_y), z(_z) { } \
	constexpr ImVec3(const ImVec2& rhs) : x(rhs.x), y(rhs.y), z(0.f) { } \
	ImVec3& operator=(const ImVec2& rhs) { x = rhs.x; y = rhs.y; return *this; } \
	operator ImVec2() const { return ImVec2(x, y); } \
    float operator[](size_t index) const { switch (index) {case 0: return x; case 1: return y; case 2: return z; default: return 0.f;} } \
    float operator[](size_t index) { switch (index) {case 0: return x; case 1: return y; case 2: return z; default: return 0.f;} } \
 \
}; \
struct ImDrawVert \
{ \
    ImVec3  pos; \
    ImVec2  uv; \
    ImU32   col; \
    float   colf[4]; \
};

#endif // __SRB2_IMCONFIG_H__
