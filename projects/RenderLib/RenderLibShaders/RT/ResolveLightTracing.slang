

#include "Common.slang"


layout(SHADER_DESCRIPTOR_BINDING + 2, rgba32f) uniform restrict RWTexture2D<float4> Target;
layout(SHADER_DESCRIPTOR_BINDING + 3) uniform restrict StructuredBuffer<float4> AccumulationBuffer;

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
		Target.Store(pixel, vec4(value, 1));
	}
}

