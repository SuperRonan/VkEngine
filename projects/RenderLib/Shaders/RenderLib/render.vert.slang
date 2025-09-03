

#define BIND_RENDERER_SET 1

#include "common.slang"


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec3 v_w_position;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec3 v_w_normal;
layout(location = 3) out vec3 v_w_tangent;

struct PushConstant
{
	AffineXForm3Df object_to_world;
}

[vk::push_constant]
uniform PushConstant _pc;

[shader("vertex")]
void main(out vec4 gl_Position : SV_Position)
{
	let camera = MakeMatrixCamera(renderer_ubo.camera);
	const mat4 w2p = camera.getWorldToProj();
	const mat4 o2w = ResizeMatrix<4, 4>(_pc.object_to_world);
	const mat4 o2p = w2p * o2w;
	
	gl_Position = o2p * vec4(a_position, 1);

	const mat3 normal_matrix = DirectionMatrix(mat3(o2w));

	v_uv = a_uv;
	v_w_normal = normalize(normal_matrix * a_normal);
	v_w_tangent = normalize(normal_matrix * a_tangent);
	v_w_position = (o2w * vec4(a_position, 1)).xyz;
}