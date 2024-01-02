
#define RENDER_MODE_2D_LIKE 0
#define RENDER_MODE_3D 1

#ifndef RENDER_MODE
#define RENDER_MODE RENDER_MODE_2D_LIKE
#endif


struct Varying
{
	vec2 uv;
#if RENDER_MODE == RENDER_MODE_3D
	vec3 world_pos;
#endif
};