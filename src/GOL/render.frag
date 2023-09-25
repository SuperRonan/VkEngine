#version 460

#include <ShaderLib:/common.glsl>

layout(SHADER_DESCRIPTOR_BINDING + 0) uniform utexture2D u_t_grid;

layout(SHADER_DESCRIPTOR_BINDING + 1) uniform sampler u_sampler;



layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec3 o_color;

void main()
{

	uint t = texture(usampler2D(u_t_grid, u_sampler), v_uv).r;
	t = t != 0 ? 1 : 0;
	o_color = vec3(t);
	
	//o_color = vec3(v_uv, 1.0f - v_uv.x - v_uv.y);

}