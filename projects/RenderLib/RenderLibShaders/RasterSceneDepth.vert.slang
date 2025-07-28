
#define SCENE_LIGHTS_ACCESS ROM 

#include <ShaderLib/Rendering/CubeMap.slang>
#include <ShaderLib/Rendering/Lights/Light.slang>


#ifndef TARGET_CUBE
#define TARGET_CUBE 0
#endif

#if TARGET_CUBE
//#extension GL_EXT_multiview : require
#endif

#include "IndirectCommon.slang"

// layout(location = 0) in vec3 a_position;
// layout(location = 1) in vec3 a_normal;
// layout(location = 2) in vec3 a_tangent;
// layout(location = 3) in vec2 a_uv;

// layout(location = 0) out vec3 v_w_position;
// layout(location = 0) out flat uvec4 v_flat;
// layout(location = 2) out vec2 v_uv;
// layout(location = 3) out vec3 v_w_normal;
// layout(location = 4) out vec3 v_w_tangent;

struct PushConstant
{
	uint light_id;
};

[vk::push_constant]
uniform PushConstant _pc;

[shader("vertex")]
vec4 main(
#if TARGET_CUBE
	const in uint view_id : SV_ViewID,
#endif
	const in uint draw_id : SV_DrawIndex,
	const in uint vertex_index : SV_VertexID
	) : SV_Position
{
	let scene = BoundScene();
	const VertexData vd = FetchIndirectVertex(scene, draw_id, vertex_index);
	let vertex = vd.vertex;

	const vec3 a_position = vertex.position;
	// const vec3 a_normal = vertex.normal;
	// const vec3 a_tangent = vertex.tangent;
	// const vec2 a_uv = vertex.uv;

	let light = SceneLights[_pc.light_id];
	
#if TARGET_CUBE
	const mat4 w2p = PointLight(light).getWorldToProj(view_id);
#else
	const mat4 w2p = SpotLight(light).getWorldToProj();
#endif
	const mat4 o2w = ResizeMatrix<4, 4>(vd.matrix);
	const mat4 o2p = w2p * o2w;
	
	vec3 m_position = a_position;
	
	vec4 gl_Position = o2p * vec4(m_position, 1);

	// const mat3 normal_matrix = mat3(o2w);

	// v_uv = a_uv;
	// // TODO Use the correct matrix (works as long as the scale is uniform)
	// v_w_normal = normal_matrix * a_normal;
	// v_w_tangent = normal_matrix * a_tangent;
	// v_w_position = (o2w * vec4(m_position, 1)).xyz;

	// v_flat.x = uint(gl_DrawID);

	return gl_Position;
}