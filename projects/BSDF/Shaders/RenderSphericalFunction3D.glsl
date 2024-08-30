#version 460

#include "Bindings.glsl"

#define TARGET_TRIANGLES 0
#define TARGET_IMPLICIT_LINES 1
#define TARGET_EXPLICIT_LINES 2

#ifndef TARGET_PRIMITIVE
#define TARGET_PRIMITIVE TARGET_TRIANGLES
#endif

#define RASTER_NORMAL_MODE_FRAGMENT_DERIVATIVE 0
#define RASTER_NORMAL_MODE_MESHLET_FACE_NORMAL 2

#ifndef RASTER_NORMAL_MODE
#define RASTER_NORMAL_MODE RASTER_NORMAL_MODE_FRAGMENT_DERIVATIVE
#endif

#define USE_VARYING_POSITION (BSDF_RENDER_MODE == BSDF_RENDER_MODE_TRANSPARENT) && (RASTER_NORMAL_MODE == RASTER_NORMAL_MODE_FRAGMENT_DERIVATIVE)

struct Varying
{
	vec4 color;
#if USE_VARYING_POSITION
	vec3 position_camera;
#endif
};

// layout(push_constant) uniform PushConstant
// {
	
// } _pc;

#if SHADER_SEMANTIC_MESH 

#extension GL_EXT_mesh_shader : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_arithmetic : require

// The Z dimension determines the index of the function

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 4

#if LOCAL_SIZE_X < 2
#error "LOCAL_SIZE_X must be at least 2"
#endif

#if LOCAL_SIZE_Y < 2
#error "LOCAL_SIZE_Y must be at least 2"
#endif

#define LOCAL_SIZE uvec2(LOCAL_SIZE_X, LOCAL_SIZE_Y)

#define LOCAL_SIZE_LINEAR (LOCAL_SIZE_X * LOCAL_SIZE_Y)
#define MAX_VERTICES_X (LOCAL_SIZE_X + 1)
#define MAX_VERTICES_Y (LOCAL_SIZE_Y + 1)
#define MAX_VERTICES (MAX_VERTICES_X * MAX_VERTICES_Y)
#define NUM_PRIMITIVE_X (LOCAL_SIZE_X)
#define NUM_PRIMITIVE_Y (LOCAL_SIZE_Y)
#define MAX_PRIMITIVES (2 * (NUM_PRIMITIVE_X * NUM_PRIMITIVE_Y))

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;
layout(triangles, max_vertices = MAX_VERTICES, max_primitives = MAX_PRIMITIVES) out;


layout(location = 0) out Varying out_v[];

struct CenterInfo
{
	uvec2 gid;
	uvec2 lid;
	vec3 wi;
	uint layer;
};

uvec2 GetResolution()
{
	const uvec2 num_wgs = gl_NumWorkGroups.xy;
	const uvec2 num_threads = num_wgs * LOCAL_SIZE;
	return num_threads;
}

vec3 GetDirection(uvec2 id)
{
	const vec2 uv = vec2(id) / vec2(GetResolution() - 1);
	const vec2 theta_phi = uv.yx * vec2(M_PI, TWO_PI);
	const vec3 direction = SphericalToCartesian(theta_phi);
	return direction;
}

float fetchIntensity(const in CenterInfo c, uvec2 offset)
{
#if BSDF_RENDER_USE_TEXTURE
	return imageLoad(bsdf_image, ivec3(c.gid + offset, c.layer)).x;
#else
	const vec3 wi = GetDirection(c.gid + offset);
	const vec3 wo = ubo.direction;
	return EvaluateSphericalFunction(c.layer, wo, wi);
#endif
}

uint fetchVertex(mat4 proj, const in CenterInfo c, uvec2 offset)
{
	const uvec2 num_wgs = gl_NumWorkGroups.xy;
	const uvec2 num_threads = num_wgs * LOCAL_SIZE;
	const uvec2 id = ModulateMaxPlusOne(c.gid + offset, num_threads);
	const vec2 uv = vec2(id) / vec2(num_threads - 1);
	const vec2 theta_phi = uv.yx * vec2(M_PI, TWO_PI);
	const float intensity = fetchIntensity(c, offset);
	const vec3 vertex_position = SphericalToCartesian(vec3(theta_phi, intensity));
	const vec4 vertex_position_h = vec4(vertex_position, 1);
	const uint index = (c.lid.y + offset.y) * MAX_VERTICES_X + (c.lid.x + offset.x);
	gl_MeshVerticesEXT[index].gl_Position = proj * vertex_position_h;
#if USE_VARYING_POSITION
	const mat4x3 world_to_cam = GetWorldToCamera();
	out_v[index].position_camera = world_to_cam * vertex_position_h;
#endif
	return index;
}

void main()
{
	const uvec2 num_wgs = gl_NumWorkGroups.xy;
	const uvec2 num_threads = num_wgs * LOCAL_SIZE;
	const uint num_functions = gl_NumWorkGroups.z;
	const uvec2 lid = gl_LocalInvocationID.xy;
	const uvec2 wid = gl_WorkGroupID.xy;
	const uvec3 gid3 = gl_GlobalInvocationID;
	const uint layer = gid3.z;
	const uvec2 gid = gid3.xy;

	CenterInfo c;
	c.gid = gid;
	c.lid = lid;
	c.layer = layer;
	c.wi = GetDirection(c.gid);
	
	const uint primitive_count = MAX_PRIMITIVES;
	const uint vertex_count = MAX_VERTICES;

	if(subgroupElect())
	{
		SetMeshOutputsEXT(vertex_count, primitive_count);
	}
	
	const mat4 world_to_proj = ubo.camera_world_to_proj;
	vec4 color = GetColor(layer);
	color.a *= ubo.common_alpha;

	uvec4 indices;
	indices.x = fetchVertex(world_to_proj, c, uvec2(0));
	out_v[indices.x].color = color;

	const bool on_edge_x = (lid.x == (LOCAL_SIZE_X - 1));
	const bool on_edge_y = (lid.y == (LOCAL_SIZE_Y - 1));
	const bool on_edge = on_edge_x || on_edge_y;


	if(on_edge_x)
	{
		indices.y = fetchVertex(world_to_proj, c, uvec2(1, 0));
		out_v[indices.y].color = color;
	}
	if(on_edge_y)
	{
		indices.z = fetchVertex(world_to_proj, c, uvec2(0, 1));
		out_v[indices.z].color = color;
	}
	if(on_edge_x && on_edge_y)
	{
		indices.w = fetchVertex(world_to_proj, c, uvec2(1, 1));
		out_v[indices.w].color = color;
	}

	else
	{
		indices.y = indices.x + MAX_VERTICES_X * 0 + 1;
		indices.z = indices.x + MAX_VERTICES_X * 1 + 0;
		indices.w = indices.x + MAX_VERTICES_X * 1 + 1;
	}
	
	{
		const uvec2 quad_id2 = lid;
		const uint quad_id = quad_id2.y * NUM_PRIMITIVE_X + quad_id2.x;
		const uint triangle_id = quad_id * 2;
		gl_PrimitiveTriangleIndicesEXT[triangle_id + 0] = indices.xyz;
		gl_PrimitiveTriangleIndicesEXT[triangle_id + 1] = indices.wzy;
	}
}

#endif

#if SHADER_SEMANTIC_FRAGMENT

layout(location = 0) in Varying in_v;

layout(location = 0) out vec4 o_color;

void main()
{
	o_color = in_v.color;

#if (BSDF_RENDER_MODE == BSDF_RENDER_MODE_TRANSPARENT)
#if RASTER_NORMAL_MODE == RASTER_NORMAL_MODE_FRAGMENT_DERIVATIVE
	const vec3 position_camera = in_v.position_camera;
	const vec3 ddx = dFdx(position_camera);
	const vec3 ddy = dFdy(position_camera);
	const vec3 normal_camera = normalize(cross(ddx, ddy));
	const vec3 wo = normalize(position_camera);
#endif
	const float a = 1.0f;
	const float b = 4.0f;
	const float mult = pow(1.0 - pow(abs(dot(normal_camera, wo)), a), b);
	o_color.a *= mult;
#endif

}

#endif