#pragma once

// For vs code cpp to understand what I write
#ifdef __cplusplus

using uint = unsigned int;
using uint32_t = uint;
using uint8_t = unsigned char;
#define in
#define out
#define inout
#define uniform
#define flat
#define buffer uint
#define sampler2D uint
#define assert(expr)
#define layout(...)

struct vec2{float x, y;};
struct vec3{float x, y, z;};
struct vec4{float x, y, z, w;};

struct ivec2{int x, y;};
struct ivec3{int x, y, z;};
struct ivec4{int x, y, z, w;};

struct uvec2{uint x, y;};
struct uvec3{uint x, y, z;};
struct uvec4{uint x, y, z, w;};

struct mat2{vec2 x, y;};
struct mat3{vec3 x, y, z;};
struct mat4{vec4 x, y, z, w;};

struct mat2x3{vec3 x, y;};
struct mat2x4{vec4 x, y;};

struct mat3x2{vec2 x, y, z;};
struct mat3x4{vec4 x, y, z;};

struct mat4x2{vec2 x, y, z, w;};
struct mat4x3{vec3 x, y, z, w;};

vec4 gl_Position;

#else
#define assert(expr)
#endif