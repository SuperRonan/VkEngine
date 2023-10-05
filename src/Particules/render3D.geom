#version 460

#include "common.glsl"
#include "render3D.glsl"

#include <ShaderLib:/Rendering/Mesh/Sphere.glsl>

#define I_WANT_TO_DEBUG 0
#include <ShaderLib:/debugBuffers.glsl>

layout(points) in;
layout(location = 0) in uint id[1];

layout(triangle_strip, max_vertices=4) out;
layout(location = 0) out flat uint v_type;
layout(location = 1) out Varying v_out;

layout(SHADER_DESCRIPTOR_BINDING + 0, std430) buffer readonly restrict b_state
{
	Particule particules[];
} state;

layout(SHADER_DESCRIPTOR_BINDING + 2) uniform UBO
{
	Uniforms3D ubo;
};

void main()
{
	const Particule p = state.particules[id[0]];
	const float radius = p.radius;

	const Sphere sphere = Sphere(p.position, radius);

	const vec3 camera_to_sphere_world = sphere.center - ubo.camera_pos;
	const float dist_camera_to_sphere_world = length(camera_to_sphere_world);
	const vec3 dir_camera_to_sphere_world = camera_to_sphere_world / dist_camera_to_sphere_world;

	const vec3 quad_center_world = sphere.center + sphere.radius * dir_camera_to_sphere_world;

	const vec4 sphere_center_proj = (ubo.world_to_proj * vec4(sphere.center, 1));
	// const vec4 sphere_center_proj = (ubo.world_to_proj * vec4(quad_center_world, 1));
	

	for(int i=0; i<2; ++i)
	{
		const float dx = float(i) - 0.5;
		for(int j=0; j<2; ++j)
		{
			const float dy = float(j) - 0.5;
			
			v_type = p.type;
			v_out.uv = vec2(dx, dy);
			const vec3 vertex_world = quad_center_world + (dx * ubo.camera_right - dy * ubo.camera_up) * radius;
			gl_Position = ubo.world_to_proj * vec4(vertex_world, 1);
			EmitVertex();
		}
	}
	EndPrimitive();

#if I_WANT_TO_DEBUG
	
	{
		Caret c = Caret3D(sphere_center_proj, 0);
		c = pushToDebugClipSpaceLn(id[0], c);
		c = pushToDebugClipSpaceLn("Position", c);
		c = pushToDebugClipSpaceLn(p.position, c);
		c = pushToDebugClipSpaceLn("Velocity", c);
		c = pushToDebugClipSpaceLn(p.velocity, c);
		c = pushToDebugClipSpaceLn("Radius", c);
		c = pushToDebugClipSpaceLn(radius, c);
		c = pushToDebugClipSpaceLn(vec4(dir_camera_to_sphere_world, length(dir_camera_to_sphere_world)), c);
	}

	if(id[0] == 0)
	{
		Caret c = Caret2D(0..xx, 0);
	// 	c = pushToDebugUVLn("Camera center", c);
	// 	c = pushToDebugUVLn(ubo.camera_pos, c);
	// 	c = pushToDebugUVLn("Camera right", c);
	// 	c = pushToDebugUVLn(ubo.camera_right, c);
	// 	c = pushToDebugUVLn("Camera up", c);
	// 	c = pushToDebugUVLn(ubo.camera_up, c);
		c = pushToDebugUVLn(quad_center_world, c);
		c = pushToDebugUVLn(sphere_center_proj, c);
		c = pushToDebugUVLn(vec4(camera_to_sphere_world, length(camera_to_sphere_world)), c);
	}

#endif
}