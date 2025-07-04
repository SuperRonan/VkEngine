#version 460

#define BIND_RENDERER_SET 1

#include "IndirectCommon.glsl"

// layout(location = 0) in vec3 a_position;
// layout(location = 1) in vec3 a_normal;
// layout(location = 2) in vec3 a_tangent;
// layout(location = 3) in vec2 a_uv;

layout(location = 0) out flat uvec4 v_flat;
layout(location = 1) out vec3 v_w_position;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_w_normal;
layout(location = 4) out vec3 v_w_tangent;

void main()
{	
	const VertexData vd = fetchVertex();
	const Vertex vertex = vd.vertex;

	const vec3 a_position = vertex.position;
	const vec3 a_normal = vertex.normal;
	const vec3 a_tangent = vertex.tangent;
	const vec2 a_uv = vertex.uv;

	const mat4 w2p = GetCameraWorldToProj(ubo.camera);
	const mat4 o2w = mat4(vd.matrix);
	const mat4 o2p = w2p * o2w;
	


	vec3 m_position = a_position;
	
	gl_Position = o2p * vec4(m_position, 1);
	if(false)
	{
		// Some tests to alter the projection
		//gl_Position.xy *= (clamp(z, 1, 10));
		//gl_Position.xy = gl_Position.xy;
		//gl_Position.z = 1.5;
		//gl_Position.xy = quantize(gl_Position.xy, 1.0 / 64);
		//gl_Position.z /= gl_Position.w;
		//gl_Position.w = 1;
	}

	const mat3 normal_matrix = mat3(o2w);

	v_uv = a_uv;
	// TODO Use the correct matrix (works as long as the scale is uniform)
	v_w_normal = normal_matrix * a_normal;
	v_w_tangent = normal_matrix * a_tangent;
	v_w_position = (o2w * vec4(m_position, 1)).xyz;

	v_flat.x = uint(gl_DrawID);
}