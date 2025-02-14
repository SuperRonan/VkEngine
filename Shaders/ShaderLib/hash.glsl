#pragma once

#include <ShaderLib:/common.glsl>
#include "TemplateImpl/default_hash.inl"

#define fast_hash_t uint32_t
#define slow_hash_t uint64_t

#if DEFAULT_HASH_POLICY == HASH_POLICY_FAST
#define hash_t fast_hash_t
#define hash fastHash
#elif DEFAULT_HASH_POLICY == HASH_POLICY_SLOW
#define hash_t slow_hash_t
#define hash slowHash
#endif

fast_hash_t fastHash(uint32_t x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

fast_hash_t fastHash(uvec2 x);

fast_hash_t fastHash(uint64_t x)
{
	fast_hash_t res = 0;
	uint32_t x0 = uint32_t(x);
	uint32_t x1 = uint32_t(x >> 32);
	uvec2 v = uvec2(x0, x1);
	return fastHash(v);
}

slow_hash_t slowHash(uint64_t x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

slow_hash_t slowHash(uint32_t x)
{
	return slowHash(uint64_t(x));
}




// Template instanciation
#define HASH_TEMPLATE_HASH_POLICY HASH_POLICY_FAST
#include "TemplateImpl/hash.glsl"
#undef HASH_TEMPLATE_HASH_POLICY

// Template instanciation
#define HASH_TEMPLATE_HASH_POLICY HASH_POLICY_SLOW
#include "TemplateImpl/hash.glsl"
#undef HASH_TEMPLATE_HASH_POLICY


