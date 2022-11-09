#version 460 core


layout(location = 0) out ivec2 vertex_id;

layout(set = 0, binding = 0) uniform sampler2D water_surface;

void main()
{
	const uint v = gl_VertexIndex;

	const ivec2 dims = textureSize(water_surface, 0);

	vertex_id.x = v % dims.x;
	vertex_id.y = v / dims.x;
}