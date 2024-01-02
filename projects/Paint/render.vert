#version 460

layout(location = 0) out int vid;

void main()
{
	vid = gl_VertexIndex;
}