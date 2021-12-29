#version 460

vec2 uvs[] = vec2[4](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));

layout(push_constant) uniform constants
{
    mat4 matrix; 
} _pc;

layout(location = 0) out vec2 v_uv;

void main()
{
	gl_Position = vec4(uvs[gl_VertexIndex], 0.0, 1.0);

	//v_uv = uvs[gl_VertexIndex] * 0.5 + vec2(0.5, 0.5);
	
	v_uv = uvs[gl_VertexIndex] * 0.5;

	v_uv = (mat3(_pc.matrix) * vec3(v_uv, 1)).xy;
}