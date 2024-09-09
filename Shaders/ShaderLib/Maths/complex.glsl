#pragma once

#include "common.glsl"

#define COMPLEX_TEMPLATE_complex_t vec2
#define COMPLEX_TEMPLATE_scalar_t float
#define COMPLEX_TEMPLATE_unsigned_scalar_t float
#define COMPLEX_TEMPLATE_real_t float
#define COMPLEX_TEMPLATE_CONTINUOUS 1
#define COMPLEX_TEMPLATE_polar_complex_t polar_complex_t

#include "TemplateImpl/complex.glsl"

#undef COMPLEX_TEMPLATE_complex_t
#undef COMPLEX_TEMPLATE_scalar_t
#undef COMPLEX_TEMPLATE_unsigned_scalar_t
#undef COMPLEX_TEMPLATE_real_t
#undef COMPLEX_TEMPLATE_CONTINUOUS
#undef COMPLEX_TEMPLATE_polar_complex_t



#define COMPLEX_TEMPLATE_complex_t uvec2
#define COMPLEX_TEMPLATE_scalar_t uint
#define COMPLEX_TEMPLATE_unsigned_scalar_t uint
#define COMPLEX_TEMPLATE_real_t float
#define COMPLEX_TEMPLATE_CONTINUOUS 0

#include "TemplateImpl/complex.glsl"

#undef COMPLEX_TEMPLATE_complex_t
#undef COMPLEX_TEMPLATE_scalar_t
#undef COMPLEX_TEMPLATE_unsigned_scalar_t
#undef COMPLEX_TEMPLATE_real_t
#undef COMPLEX_TEMPLATE_CONTINUOUS



#define COMPLEX_TEMPLATE_complex_t ivec2
#define COMPLEX_TEMPLATE_scalar_t int
#define COMPLEX_TEMPLATE_unsigned_scalar_t uint
#define COMPLEX_TEMPLATE_real_t float
#define COMPLEX_TEMPLATE_CONTINUOUS 0

#include "TemplateImpl/complex.glsl"

#undef COMPLEX_TEMPLATE_complex_t
#undef COMPLEX_TEMPLATE_scalar_t
#undef COMPLEX_TEMPLATE_unsigned_scalar_t
#undef COMPLEX_TEMPLATE_real_t
#undef COMPLEX_TEMPLATE_CONTINUOUS





#define complex_t vec2
#define cx_t complex_t
#define ucomplex_t uvec2
#define icomplex_t ivec2
#define dcomplex_t dvec2

