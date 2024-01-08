#pragma once

uint fastHash(uint x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

uint hash(uint x)
{
	return fastHash(x);
}

uint hash(int x)
{
	return hash(uint(x));
}

uint hash(uvec2 v)
{
	return hash(v.x ^ hash(v.y));
}

uint hash(ivec2 v)
{
	return hash(uvec2(v));
}

#define RNGState uint

uint randomUint(inout RNGState rng)
{
	uint res = hash(rng);
	++rng;
	return res;
}

float randomFloat01(inout RNGState rng)
{
	const uint mask = 0x0000ffff;
	const uint tmp = hash(rng) & mask;
	const float res = float(tmp) / float(mask);
	++rng;
	return res;
}

int randomSigni(inout RNGState rng)
{
	int r = int(randomUint(rng));
	int res = (r & 1) * 2 - 1;
	return res;
}

float randomSignf(inout RNGState rng)
{
	return float(randomSigni(rng));
}

float randomFloat(inout RNGState rng, float lower, float upper)
{
	const float xi = randomFloat01(rng);
	return (xi * (upper - lower)) + lower; 
}

vec2 randomVec2_01(inout RNGState rng)
{
	return vec2(randomFloat01(rng), randomFloat01(rng));
}

vec2 randomUniformPolar(inout RNGState rng)
{
	vec2 xi = randomVec2_01(rng);
	float r = sqrt(xi.x);
	float t = TWO_PI * xi.y;
	return r * vec2(cos(t), sin(t));
}

vec3 randomRGB(inout RNGState rng)
{
	return vec3(
		randomFloat01(rng),
		randomFloat01(rng),
		randomFloat01(rng)
	);
}

vec3 RGBFromIndex(uint id)
{
	return randomRGB(id);
}