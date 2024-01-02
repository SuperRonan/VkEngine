#version 460

struct Vertex
{
	vec3 position;
	vec3 speed;
	vec3 normal;
	float mass;
};

layout(set = 0, binding = 0, std430) uniform restrict readonly buffer prev_uniform
{
	const Vertex vertices[];
} prev;

layout(push_constant, std430) uniform push_constants
{
	ivec2 dims;
	float spring_length;
    mat4 ObjectToProj;
} pc;