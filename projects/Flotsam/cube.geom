
#version 460 core

#include "common.glsl"

#line 7

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

layout(location = 0) in uint cube_id[1];
layout(location = 1) in vec3 face_normal[1]; 
layout(location = 2) in vec3 face_tangent[1]; 

layout(location = 0) out vec3 world_position;
layout(location = 1) out vec3 world_normal;
layout(location = 2) out vec2 o_uv;

layout(set = 0, binding = 0) buffer readonly restrict CubeBuffer
{
	CubeState[NUMBER_OF_CUBES * 2] states;
} cubes;

layout(push_constant) uniform PushConstants
{
	mat4 world2proj;
	uint flags;
} _pc;


void emitVert
(
	CubeState cube, 
	vec2 uv
	)
{
	const vec3 face_ctan = cross(face_normal[0], face_tangent[0]);
	world_position = 0..xxx
		+ cube.center_position + 0.5 * face_normal[0]
		+ mix(-0.5, 0.5, uv.x) * face_tangent[0] 
		+ mix(-0.5, 0.5, uv.y) * face_ctan
	;

	world_normal = face_normal[0];

	o_uv = uv;

	gl_Position = _pc.world2proj * vec4(world_position, 1.0);

	EmitVertex(); 
		
}

void main()
{
	const uint buffer_id = GetCurrentBufferIndex(cube_id[0], _pc.flags);

	const CubeState cube = cubes.states[buffer_id];

	const vec3 face_ctan = cross(face_normal[0], face_tangent[0]);

	for(int i = 0; i < 2; ++i)
	{
		for(int j = 0; j < 2; ++j)
		{
			emitVert(cube, vec2(i, j));

		}
	}
	
	EndPrimitive();
}