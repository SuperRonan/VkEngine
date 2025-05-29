
#define BIND_SCENE 1
#define BIND_RENDERER_SET 1

#include "common.slang"

#include "IndirectCommon.slang"

#include <ShaderLib/Maths/transforms.slang>

// layout(location = 0) in vec3 a_position;
// layout(location = 1) in vec3 a_normal;
// layout(location = 2) in vec3 a_tangent;
// layout(location = 3) in vec2 a_uv;

layout(location = 0) out nointerpolation uvec4 v_flat;
layout(location = 1) out vec3 v_w_position;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_w_normal;
layout(location = 4) out vec3 v_w_tangent;

[shader("vertex")]
void main(
	out vec4 gl_Position : SV_Position, 
	const in uint draw_id : SV_DrawIndex, 
	const in uint vertex_index : SV_VertexID
) {
	BoundScene scene;
	const VertexData vd = FetchIndirectVertex(scene, draw_id, vertex_index);
	let vertex = vd.vertex;

	const vec3 a_position = vertex.position;
	const vec3 a_normal = vertex.normal;
	const vec3 a_tangent = vertex.tangent;
	const vec2 a_uv = vertex.uv;
	
	let camera = MakeMatrixCamera(renderer_ubo.camera);
	const mat4 w2p = camera.getWorldToProj();
	let o2w = vd.matrix;
	const mat4 o2p = w2p * ResizeMatrix<4, 4>(o2w);


	gl_Position = o2p * vec4(a_position, 1);

	const mat3 normal_matrix = DirectionMatrix(ExtractQBlock(o2w));

	v_uv = a_uv;
	v_w_normal = Normalize(normal_matrix * a_normal);
	v_w_tangent = Normalize(normal_matrix * a_tangent);
	v_w_position = (o2w * vec4(a_position, 1)).xyz;

	v_flat.x = draw_id;

}