#ifndef Random_INCLUDED
#define Random_INCLUDED

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

float randUint(inout uint rng)
{
	uint res = hash(rng);
	++rng;
	return res;
}

float randFloat01(inout uint rng)
{
	const uint mask = 0x0000ffff;
	const uint tmp = hash(rng) & mask;
	const float res = float(tmp) / float(mask);
	++rng;
	return res;
}

float randFloat(inout uint rng, float lower, float upper)
{
	const float xi = randFloat01(rng);
	return (xi * (upper - lower)) + lower; 
}

#endif