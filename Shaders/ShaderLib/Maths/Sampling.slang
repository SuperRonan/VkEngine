#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/Transforms.slang>

struct Sample<T>
{
	T value;
	float pdf ;
}

vec3 DiskToHemisphere(vec2 on_disk)
{
	return vec3(on_disk, sqrt(1 - Length2(on_disk)));
}

// Sample a point on a unit disk
Sample<vec2> SampleUniformOnDisk(vec2 xi)
{
	Sample<vec2> res;
	const float rho = sqrt(xi.x);
	const float theta = TWO_PI * xi.y;
	res.value = rho * vec2(cos(theta), sin(theta));
	res.pdf = oo_PI;
	return res;
}

// Sample a direction on a +z unit hemisphere ~ a cosine distribution 
Sample<vec3> SampleCosineDirection(vec2 xi)
{
	const vec2 disk = SampleUniformOnDisk(xi).value;
	Sample<vec3> res;
	res.value = DiskToHemisphere(disk);
	res.pdf = res.value.z * oo_PI;
	return res;
}

// Sample a uniform distribution on a sphere
Sample<vec3> SampleUniformDirection(vec2 xi)
{
	// Seems correct
	Sample<vec3> res;
	const float z = (2 * xi.x - 1);
	const float radius = sqrt(1 - sqr(z));
	const float phi = TWO_PI * xi.y;
	const vec2 point_on_circle = vec2(cos(phi), sin(phi));
	res.value = vec3(point_on_circle * radius, z);
	res.pdf = 0.25 * oo_PI;
	return res;
}

