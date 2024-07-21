#version 460

#include "Bindings.glsl"

struct Varying
{
	vec4 color;
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


float fetchIntensity(uvec2 id, uint layer)
{
	//imageLoad(bsdf_image, ivec3(gid, gl_GlobalInvocationID.z)).x;
	const uvec2 num_wgs = gl_NumWorkGroups.xy;
	const uvec2 num_threads = num_wgs * LOCAL_SIZE;
	const vec2 uv = vec2(id) / vec2(num_threads - 1);
	const vec2 theta_phi = uv.yx * vec2(M_PI, TWO_PI);
	const vec3 direction = SphericalToCartesian(theta_phi);
	return pow(direction.y, 10);
}

uint fetchVertex(mat4 proj, uvec2 lid, uvec2 gid, uint layer, uvec2 offset)
{
	const uvec2 num_wgs = gl_NumWorkGroups.xy;
	const uvec2 num_threads = num_wgs * LOCAL_SIZE;
	const uvec2 id = ModulateMaxPlusOne(gid + offset, num_threads);
	const vec2 uv = vec2(id) / vec2(num_threads - 1);
	const vec2 theta_phi = uv.yx * vec2(M_PI, TWO_PI);
	const float intensity = fetchIntensity(id, layer);
	const vec3 vertex_position = SphericalToCartesian(vec3(theta_phi, intensity));
	const uint index = (lid.y + offset.y) * MAX_VERTICES_X + (lid.x + offset.x);
	gl_MeshVerticesEXT[index].gl_Position = proj * vec4(vertex_position, 1);
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
	
	const uint primitive_count = MAX_PRIMITIVES;
	const uint vertex_count = MAX_VERTICES;

	if(subgroupElect())
	{
		SetMeshOutputsEXT(vertex_count, primitive_count);
	}
	
	const mat4 world_to_proj = ubo.camera_world_to_proj;
	vec4 color = vec4(1, 1, 1, 1);
	color.a *= ubo.common_alpha;

	uvec4 indices;
	indices.x = fetchVertex(world_to_proj, lid, gid, layer, uvec2(0));
	out_v[indices.x].color = color;

	const bool on_edge_x = (lid.x == (LOCAL_SIZE_X - 1));
	const bool on_edge_y = (lid.y == (LOCAL_SIZE_Y - 1));
	const bool on_edge = on_edge_x || on_edge_y;


	if(on_edge_x)
	{
		indices.y = fetchVertex(world_to_proj, lid, gid, layer, uvec2(1, 0));
		out_v[indices.y].color = color;
	}
	if(on_edge_y)
	{
		indices.z = fetchVertex(world_to_proj, lid, gid, layer, uvec2(0, 1));
		out_v[indices.z].color = color;
	}
	if(on_edge_x && on_edge_y)
	{
		indices.w = fetchVertex(world_to_proj, lid, gid, layer, uvec2(1, 1));
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

}

#endif