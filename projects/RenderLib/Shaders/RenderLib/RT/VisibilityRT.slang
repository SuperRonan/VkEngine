
#define BIND_SCENE 1
#define BIND_SCENE_TLAS 1
#define BIND_ALL_SCENE_COMPONENTS 1

#include "Common.slang"

#if SHADER_SEMANTIC_MISS
[shader("miss")]
void main(inout float payload)
{
	payload = 1;
}
#endif

#if SHADER_SEMANTIC_CLOSEST_HIT
[shader("closesthit")]
void main(inout float payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload = 0;
}
#endif

#if SHADER_SEMANTIC_ANY_HIT
[shader("anyhit")]
void main(inout float payload, in BuiltInTriangleIntersectionAttributes attr)
{
	// BoundScene scene;
	// vec2 bary = attr.barycentrics;
	// uint object_index = InstanceID();
	// uint triangle_index = PrimitiveIndex();
	// if(!scene.TestTriangleOpacity<true>(object_index, triangle_index, bary))
	// {
	// 	IgnoreHit();
	// }
}
#endif




