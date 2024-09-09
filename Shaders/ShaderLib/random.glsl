#pragma once

#include "hash.glsl"

#define fast_rng_t fast_hash_t
#define slow_rng_t slow_hash_t

#define rng_t hash_t

// Instanciate template 
#define RANDOM_TEMPLATE_POLICY HASH_POLICY_FAST
#include "TemplateImpl/random.glsl"
#undef RANDOM_TEMPLATE_POLICY 

// Instanciate template 
#define RANDOM_TEMPLATE_POLICY HASH_POLICY_SLOW
#include "TemplateImpl/random.glsl"
#undef RANDOM_TEMPLATE_POLICY





vec3 RGBFromIndex(uint id)
{
	return randomRGB(id);
}