#pragma once

uint hash(uint x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

uint hash(uvec2 v)
{
	return hash(v.x ^ hash(v.y));
}

uint randomUint(inout uint rng)
{
	uint res = hash(rng);
	++rng;
	return res;
}

float randomFloat01(inout uint rng)
{
	const uint mask = 0x0000ffff;
	const uint tmp = hash(rng) & mask;
	const float res = float(tmp) / float(mask);
	++rng;
	return res;
}

float randomFloat(inout uint rng, float lower, float upper)
{
	const float xi = randomFloat01(rng);
	return (xi * (upper - lower)) + lower; 
}

vec3 randomRGB(inout uint rng)
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