#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/Transforms.slang>

struct Sample<T>
{
	T value;
	float pdf ;
}

// Sample a point on a unit disk
Sample<vec2> SampleUniformOnDisk(vec2 xi)
{
	Sample<vec2> res;
	const float rho = sqrt(xi.x);
	const float theta = TWO_PI * xi.y;
	res.value = rho * vec2(cos(theta), sin(theta));
	res.pdf = rcp(PI);
	return res;
}

// Sample a direction on a +z unit hemisphere ~ a cosine distribution 
Sample<vec3> SampleCosineDirection(vec2 xi)
{
	const vec2 disk = SampleUniformOnDisk(xi).value;
	Sample<vec3> res;
	res.value = vec3(disk, sqrt(1 - Length2(disk)));
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
	res.pdf = rcp(4.0f * PI);
	return res;
}

