#pragma once

#include "common.glsl"


#define QUATERNION_TEMPLATE_quaternion_t vec4
#define QUATERNION_TEMPLATE_scalar_t float
#define QUATERNION_TEMPLATE_unsigned_scalar_t float
#define QUATERNION_TEMPLATE_vec3 vec3
#define QUATERNION_TEMPLATE_real_t float
#define QUATERNION_TEMPLATE_CONTINUOUS 1
#define QUATERNION_TEMPLATE_polar_quaternion_t polar_quaternion_t

#include "TemplateImpl/quaternion.glsl"

#undef QUATERNION_TEMPLATE_quaternion_t 
#undef QUATERNION_TEMPLATE_scalar_t 
#undef QUATERNION_TEMPLATE_unsigned_scalar_t 
#undef QUATERNION_TEMPLATE_vec3 
#undef QUATERNION_TEMPLATE_real_t 
#undef QUATERNION_TEMPLATE_CONTINUOUS 
#undef QUATERNION_TEMPLATE_polar_quaternion_t 



#define qx_add quaternion_add
#define qx_sub quaternion_sub
#define qx_mul quaternion_mul
#define qx_rcp quaternion_rcp
#define qx_div quaternion_div
#define qx_srq quaternion_sqr
#define qx_sqrt quaternion_sqrt

#define qx_exp quaternion_exp
#define qx_log quaternion_log
#define qx_pow quaternion_pow


