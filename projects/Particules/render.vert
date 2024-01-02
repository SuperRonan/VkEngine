#version 460

layout(location = 0) out uint id;

void main()
{
    id = gl_VertexIndex;
}