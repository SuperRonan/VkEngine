
#ifndef HASH_POLICY_FAST
#define HASH_POLICY_FAST 0
#endif
#ifndef HASH_POLICY_SLOW
#define HASH_POLICY_SLOW 1
#endif

#ifndef HASH_TEMPLATE_HASH_POLICY
//#error "HASH_TEMPLATE_HASH_POLICY not defined"
#define HASH_TEMPLATE_HASH_POLICY HASH_POLICY_FAST
#endif 

#if HASH_TEMPLATE_HASH_POLICY == HASH_POLICY_FAST
#define HASH_TEMPLATE_hash_t fast_hash_t
#define HASH_TEMPLATE_hash_fn fastHash
#elif HASH_TEMPLATE_HASH_POLICY == HASH_POLICY_SLOW
#define HASH_TEMPLATE_hash_t slow_hash_t
#define HASH_TEMPLATE_hash_fn slowHash
#endif

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(int x)
{
	return HASH_TEMPLATE_hash_fn(uint(x));
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(float x)
{
	return HASH_TEMPLATE_hash_fn(floatBitsToUint(x));
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(uvec2 v)
{
	return HASH_TEMPLATE_hash_fn(HASH_TEMPLATE_hash_t(v.x) ^ HASH_TEMPLATE_hash_fn(v.y));
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(uvec3 v)
{
	HASH_TEMPLATE_hash_t res = HASH_TEMPLATE_hash_fn(HASH_TEMPLATE_hash_t(v.x) ^ HASH_TEMPLATE_hash_fn(v.y));
	res = HASH_TEMPLATE_hash_fn(res ^ HASH_TEMPLATE_hash_fn(v.z));
	return res;
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(uvec4 v)
{
	HASH_TEMPLATE_hash_t res = HASH_TEMPLATE_hash_fn(v.xy) ^ HASH_TEMPLATE_hash_fn(v.zw);
	return HASH_TEMPLATE_hash_fn(res);
}


HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(ivec2 v)
{
	return HASH_TEMPLATE_hash_fn(uvec2(v));
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(ivec3 v)
{
	return HASH_TEMPLATE_hash_fn(uvec3(v));
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(ivec4 v)
{
	return HASH_TEMPLATE_hash_fn(uvec4(v));
}



HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(vec2 v)
{
	uvec2 u;
	for(int i=0; i<2; ++i)
	{
		u[i] = floatBitsToUint(v[i]);
	}
	return HASH_TEMPLATE_hash_fn(u);
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(vec3 v)
{
	uvec3 u;
	for(int i=0; i<3; ++i)
	{
		u[i] = floatBitsToUint(v[i]);
	}
	return HASH_TEMPLATE_hash_fn(u);
}

HASH_TEMPLATE_hash_t HASH_TEMPLATE_hash_fn(vec4 v)
{
	uvec4 u;
	for(int i=0; i<4; ++i)
	{
		u[i] = floatBitsToUint(v[i]);
	}
	return HASH_TEMPLATE_hash_fn(u);
}


#undef HASH_TEMPLATE_hash_t
#undef HASH_TEMPLATE_hash_fn