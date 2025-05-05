
#include "Common.slang"

typealias TargetFormat = float4;

#define RESOLVE_MODE_OVERWRITE 0
#define RESOLVE_MODE_ADD 1

#ifndef RESOLVE_MODE
#define RESOLVE_MODE 0
#endif



#if RESOLVE_MODE == RESOLVE_MODE_OVERWRITE
typealias TargetTexture = WTexture2D<TargetFormat>;
#else
typealias TargetTexture = RWTexture2D<TargetFormat>;
#endif

layout(SHADER_DESCRIPTOR_BINDING + 2, rgba32f) uniform restrict TargetTexture Target;
layout(SHADER_DESCRIPTOR_BINDING + 3) uniform restrict StructuredBuffer<Vector3<LightBufferAccumFloat>> AccumulationBuffer;

struct PushConstant
{
	uint value;
};

#define LOCAL_SIZE_X 16
#define LOCAL_SIZE_Y 16
#define LOCAL_SIZE_Z 1


[shader("compute")]
[numthreads(LOCAL_SIZE_X, LOCAL_SIZE_Y, LOCAL_SIZE_Z)]
void main(const uvec3 GlobalInvocationID : SV_DispatchThreadID)
{
	const uvec2 pixel = GlobalInvocationID.xy;
	const uvec2 dims = TextureSize(Target);
	const vec2 oo_dims = rcp(vec2(dims));
	if(all(pixel < dims))
	{
		uint address = pixel.x + pixel.y * dims.x;
		vec3 value = AccumulationBuffer[address].xyz;
#if RESOLVE_MODE == RESOLVE_MODE_OVERWRITE
		Target.Store(pixel, vec4(value, 0));
#elif RESOLVE_MODE == RESOLVE_MODE_ADD
		Target[pixel] += vec4(value, 0);
#endif
	}
}

