#version 460

#include "common.glsl"
#include "render3D.glsl"

#include <ShaderLib:/Rendering/Mesh/Sphere.glsl>

#define I_WANT_TO_DEBUG 0
#include <ShaderLib:/debugBuffers.glsl>

layout(points) in;
layout(location = 0) in uint id[1];

layout(triangle_strip, max_vertices=4) out;
layout(location = 0) out flat uvec2 v_type_particule_index;
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

#if RENDER_MODE == RENDER_MODE_2D_LIKE
	const vec3 quad_center_world = sphere.center;
#elif RENDER_MODE == RENDER_MODE_3D
	const vec3 quad_center_world = sphere.center - sphere.radius * dir_camera_to_sphere_world;
#endif

	const vec4 sphere_center_proj = (ubo.world_to_proj * vec4(sphere.center, 1));
	// const vec4 sphere_center_proj = (ubo.world_to_proj * vec4(quad_center_world, 1));
#if RENDER_MODE == RENDER_MODE_2D_LIKE
	const vec3 quad_right = ubo.camera_right;
	const vec3 quad_up = ubo.camera_up;
#elif RENDER_MODE == RENDER_MODE_3D
	const vec3 quad_right = normalize(cross(dir_camera_to_sphere_world, ubo.camera_up));
	const vec3 quad_up = -normalize(cross(dir_camera_to_sphere_world, quad_right));
#endif

// TODO 3D:
// The current quads are conservatively big enough to view the sphere, but it can be smaller 
// generate smaller quad since the sphere is behind is thus closer 

	for(int i=0; i<2; ++i)
	{
		const float dx = float(i) - 0.5;
		for(int j=0; j<2; ++j)
		{
			const float dy = float(j) - 0.5;
			
			v_type_particule_index = uvec2(p.type, id[0]);
			v_out.uv = vec2(dx, dy);
			const vec3 vertex_world = quad_center_world + (dx * quad_right - dy * quad_up) * (radius * 2.0f);
#if RENDER_MODE == RENDER_MODE_3D
			v_out.world_pos = vertex_world;
#endif
			gl_Position = ubo.world_to_proj * vec4(vertex_world, 1);
			EmitVertex();
		}
	}
	EndPrimitive();

#if I_WANT_TO_DEBUG
	
	if(false)
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
		// c = pushToDebugUVLn(quad_center_world, c);
		// c = pushToDebugUVLn(sphere_center_proj, c);
		// c = pushToDebugUVLn(vec4(camera_to_sphere_world, length(camera_to_sphere_world)), c);
		c = pushToDebugUVLn(quad_right, c);
		c = pushToDebugUVLn(quad_up, c);
	}

#endif
}