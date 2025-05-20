#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/Maths/Transforms.slang>
#include <ShaderLib/Maths/Rotations.slang>

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

float GetIsoscelesTriangleUniformPdf(float half_angle)
{
	return 0.5 * 1; // TODO
}

Sample<vec2> SampleUniformOnIsoscelesTriangle(vec2 xi, float half_angle)
{
	vec2 uv = xi;
	if(uv.x + uv.y > 1)
	{
		uv = 1.0 - uv;
	}
	const mat2 edges = MakeFromCols(
		vec2(cos(-half_angle), sin(-half_angle)), 
		vec2(cos(half_angle), sin(half_angle)));
	Sample<vec2> res;
	res.value = edges * uv;
	res.pdf = GetIsoscelesTriangleUniformPdf(half_angle);
	return res;
}

float GetEquilateralTriangleUniformPDF()
{
	const float side2 = 3.0f;
	return sqrt(3) * side2 * 0.25;
}

Sample<vec2> SampleUniformOnEquilateralTriangle(vec2 xi)
{
	const float half_angle = PI / 6;
	const mat2 edges = MakeFromCols(
		vec2(cos(-half_angle), sin(-half_angle)), 
		vec2(cos(half_angle), sin(half_angle)));
	const vec2 base = -edges * vec2(1.0 / 3.0);
	var res = SampleUniformOnIsoscelesTriangle(xi, half_angle);
	res.value = (res.value + base) * 2;
	res.pdf = GetEquilateralTriangleUniformPDF();
	return res;
}

float GetRegularPolygonUniformPDF(uint sides)
{
	float res = 0;
	if(sides == 0)
	{
		res = oo_PI;
	}
	else if(sides == 1)
	{
		res = 0.5f;
	}
	else if(sides == 3)
	{
		res = GetEquilateralTriangleUniformPDF();
	}
	else
	{
		float opening_angle = TWO_PI / float(sides);
		float half_angle = opening_angle * 0.5;
		const float area = 2 * sin(half_angle) * cos(half_angle);
		res = rcp(area);
	}
	return res;
}

Sample<vec2> SampleUniformOnRegularPolygon(vec2 xi, uint sides)
{
	Sample<vec2> res;
	if(sides == 0)
	{
		res = SampleUniformOnDisk(xi);
	}
	else if(sides == 1)
	{
		res.value.x = xi.x * 2.0f - 1.0f;
		res.value.y = 0;
		res.pdf = 0.5f;
	}
	else if(sides == 3)
	{
		res = SampleUniformOnEquilateralTriangle(xi);
	}
	else 
	{
		float xn = xi.x * sides;
		float side = floor(xn);
		xi.x = xn - side;
		float angle = side * TWO_PI / float(sides);
		mat2 R = Rotation2(angle);
		float opening_angle = TWO_PI / float(sides);
		float half_angle = opening_angle * 0.5;
		let on_triangle = SampleUniformOnIsoscelesTriangle(xi, half_angle);
		res.value = R * on_triangle.value;
		const float area = 2 * sin(half_angle) * cos(half_angle);
		res.pdf = rcp(area);
	}
	return res;
}

