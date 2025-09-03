
#define BIND_SCENE 1
#define BIND_SCENE_TLAS 1
#define BIND_ALL_SCENE_COMPONENTS 1

#include "Common.slang"

typealias Payload = TracePayload;

#if SHADER_SEMANTIC_MISS
[shader("miss")]
void main(inout Payload payload)
{
	payload.flags = uint(Hit::Flags::None);
}
#endif

#if SHADER_SEMANTIC_CLOSEST_HIT
[shader("closesthit")]
void main(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.flags = uint(Hit::Flags::Triangle);
	payload.flags = payload.flags | (((HitKind() & HIT_KIND_TRIANGLE_FRONT_FACE) != 0) ? Hit::Flags::FontFacing : Hit::Flags::BackFacing);
	
	payload.custom_index = InstanceID();
	payload.primitive_index = PrimitiveIndex();
	payload.uv_pack = packUnorm2x16(attr.barycentrics);
}

#endif

#if SHADER_SEMANTIC_ANY_HIT
[shader("anyhit")]
void main(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
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
