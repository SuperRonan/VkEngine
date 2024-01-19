
#ifndef RANDOM_POLICY_FAST
#define RANDOM_POLICY_FAST 0
#endif

#ifndef RANDOM_POLICY_SLOW
#define RANDOM_POLICY_SLOW 1
#endif

#ifndef RANDOM_TEMPLATE_POLICY
#error "RANDOM_TEMPLATE_POLICY not defined!"
#endif


#if RANDOM_TEMPLATE_POLICY == RANDOM_POLICY_FAST
#define RANDOM_TEMPLATE_rng_t fast_rng_t
#define RANDOM_TEMPLATE_hash_fn fastHash
#elif RANDOM_TEMPLATE_POLICY == RANDOM_POLICY_SLOW
#define RANDOM_TEMPLATE_rng_t slow_rng_t
#define RANDOM_TEMPLATE_hash_fn slowHash
#endif

uint32_t randomU32(inout RANDOM_TEMPLATE_rng_t rng)
{
	uint32_t res = uint32_t(RANDOM_TEMPLATE_hash_fn(rng));
	++rng;
	return res;
}

uint64_t randomU64(inout RANDOM_TEMPLATE_rng_t rng)
{
	uint64_t res = 0;
#if RANDOM_TEMPLATE_POLICY == RANDOM_POLICY_FAST
	uint32_t a = randomU32(rng);
	uint32_t b = randomU32(rng);
	res = uint64_t(a) << 32 | uint64_t(b);
#elif RANDOM_TEMPLATE_POLICY == RANDOM_POLICY_SLOW
	res = RANDOM_TEMPLATE_hash_fn(rng);
	++rng;
#endif
	return res;
}


uint randomUint(inout RANDOM_TEMPLATE_rng_t rng)
{
	return randomU32(rng);
}

uint randomUint(inout RANDOM_TEMPLATE_rng_t rng, uint lower, uint upper)
{
	const uint diff = upper - lower;
	uint res = randomUint(rng);
	res = (res % diff) + lower;
	return res;
}

float randomFloat01(inout RANDOM_TEMPLATE_rng_t rng)
{
	const uint mask = 0x00ffffff;
	const uint tmp = randomUint(rng) & mask;
	const float res = float(tmp) / float(mask);
	return res;
}

int randomSigni(inout RANDOM_TEMPLATE_rng_t rng)
{
	int r = int(randomUint(rng));
	int res = (r & 1) * 2 - 1;
	return res;
}

float randomSignf(inout RANDOM_TEMPLATE_rng_t rng)
{
	return float(randomSigni(rng));
}

float randomFloat(inout RANDOM_TEMPLATE_rng_t rng, float lower, float upper)
{
	const float xi = randomFloat01(rng);
	return (xi * (upper - lower)) + lower; 
}

vec2 randomVec2_01(inout RANDOM_TEMPLATE_rng_t rng)
{
	return vec2(randomFloat01(rng), randomFloat01(rng));
}

vec2 randomUniformPolar(inout RANDOM_TEMPLATE_rng_t rng)
{
	vec2 xi = randomVec2_01(rng);
	float r = sqrt(xi.x);
	float t = TWO_PI * xi.y;
	return r * vec2(cos(t), sin(t));
}

vec3 randomRGB(inout RANDOM_TEMPLATE_rng_t rng)
{
	return vec3(
		randomFloat01(rng),
		randomFloat01(rng),
		randomFloat01(rng)
	);
}

#undef RANDOM_TEMPLATE_rng_t
#undef RANDOM_TEMPLATE_hash_fn