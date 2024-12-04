#ifndef TARGET_FORMAT
//#error "TARGET_FORMAT no defined!"
#define TARGET_FORMAT "rgba16f"
#endif

#include <ShaderLib/common.slang>

float3 filter(float3 v)
{
	v.x = cos(v.x * 10) * 0.5 + 0.5;
	v.y = sin(v.y * 10) * 0.5 + 0.5;
	v.z = abs(sin(v.z * 20 + 3.14 / 4));
	return v;
}

// [[vk::binding(0, 3)]]
// [[vk::image_format("rgba16f")]]

layout(set = 3, binding = 0, rgba16f)
RWTexture2D<float4> Target;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
	uint2 pixel = threadId.xy;
	uint2 dims;
	uint mips;
	Target.GetDimensions(dims.x, dims.y);

	if (all(pixel < dims))
	{
		float4 v = Target.Load(pixel);
		v.xyz = filter(v.xyz);
		Target[pixel] = v;
	}
}