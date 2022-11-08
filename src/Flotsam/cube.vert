#version 460 core

layout(location = 0) out uint cube_id;
layout(location = 1) out vec3 face_normal; 
layout(location = 2) out vec3 face_tangent;


void main()
{
	const uint v = gl_VertexIndex;

	cube_id = v / 6;

	face_normal = 0..xxx;
	
	const uint face_id = (v % 6);

	face_normal[face_id / 2] = (((v% 2) == 0) ? -1.0 : 1.0);
	
	face_tangent = face_normal.zxy;
}